/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DesignatedControllerConnection.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/HealthCheckedConnection.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>
#include <ola/rdm/RDMCommandSerializer.h>

#include <memory>
#include <string>

#include "plugins/e131/e131/ACNVectors.h"
#include "plugins/e131/e131/E133Header.h"
#include "plugins/e131/e131/E133PDU.h"
#include "plugins/e131/e131/E133StatusInflator.h"

#include "tools/e133/DesignatedControllerConnection.h"
#include "tools/e133/E133Endpoint.h"
#include "tools/e133/E133HealthCheckedConnection.h"
#include "tools/e133/EndpointManager.h"
#include "tools/e133/TCPConnectionStats.h"

using ola::NewCallback;
using ola::io::IOStack;
using ola::network::HealthCheckedConnection;
using ola::plugin::e131::TransportHeader;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using std::auto_ptr;
using std::string;


/**
 * Create a new DesignatedControllerConnection.
 * This listens for connections from the controllers, and will ensure that if
 * any controllers try to connect, at least one will be picked as the
 * designated controller.
 */
DesignatedControllerConnection::DesignatedControllerConnection(
    ola::io::SelectServerInterface *ss,
    const ola::network::IPV4Address &ip_address,
    MessageBuilder *message_builder,
    TCPConnectionStats *tcp_stats)
    : m_ip_address(ip_address),
      m_ss(ss),
      m_message_builder(message_builder),
      m_tcp_stats(tcp_stats),
      m_tcp_socket(NULL),
      m_health_checked_connection(NULL),
      m_message_queue(NULL),
      m_tcp_message_sender(m_message_builder),
      m_incoming_tcp_transport(NULL),
      m_tcp_socket_factory(
          NewCallback(this, &DesignatedControllerConnection::NewTCPConnection)),
      m_listening_tcp_socket(&m_tcp_socket_factory),
      m_root_inflator(
          NewCallback(this, &DesignatedControllerConnection::RLPDataReceived)) {
  m_root_inflator.AddInflator(&m_e133_inflator);
  m_e133_inflator.AddInflator(&m_e133_status_inflator);

  m_e133_status_inflator.SetStatusHandler(
      NewCallback(this, &DesignatedControllerConnection::HandleStatusMessage));
}


DesignatedControllerConnection::~DesignatedControllerConnection() {
  m_ss->RemoveReadDescriptor(&m_listening_tcp_socket);
  m_listening_tcp_socket.Close();

  TCPConnectionClosed();
}


/**
 * Init.
 */
bool DesignatedControllerConnection::Init() {
  // setup the TCP socket
  bool listen_ok = m_listening_tcp_socket.Listen(
      IPV4SocketAddress(m_ip_address, ola::plugin::e131::E133_PORT));
  if (!listen_ok) {
    m_listening_tcp_socket.Close();
    return false;
  }

  // And add to the Select Server
  m_ss->AddReadDescriptor(&m_listening_tcp_socket);
  return true;
}


/**
 * Send an unsolicated RDM message on the TCP channel.
 * @param command the RDMResponse to send, ownership is transferred.
 */
bool DesignatedControllerConnection::SendStatusMessage(
    const ola::rdm::RDMResponse *response) {
  bool ok = m_tcp_message_sender.AddMessage(ROOT_E133_ENDPOINT, response);
  if (!ok) {
    delete response;
  }
  return ok;
}


/**
 * Force close the master's TCP connection.
 * @return, true if there was a connection to close, false otherwise.
 */
bool DesignatedControllerConnection::CloseTCPConnection() {
  if (!m_tcp_socket)
    return false;

  ola::io::ConnectedDescriptor::OnCloseCallback *callback =
    m_tcp_socket->TransferOnClose();
  callback->Run();
  return true;
}


/**
 * Called when we get a new TCP connection.
 */
void DesignatedControllerConnection::NewTCPConnection(
    ola::network::TCPSocket *socket_ptr) {
  auto_ptr<ola::network::TCPSocket> socket(socket_ptr);
  ola::network::GenericSocketAddress addr = socket->GetPeer();
  if (addr.Family() != AF_INET) {
    OLA_WARN << "New TCP connection but failed to determine peer address";
    return;
  }
  IPV4SocketAddress v4_address = addr.V4Addr();
  OLA_INFO << "New TCP connection from " << v4_address;

  if (m_tcp_socket) {
    OLA_WARN << "Already got a TCP connection open, closing this one";
    socket->Close();
    return;
  }

  m_tcp_socket = socket.release();
  if (m_message_queue)
    OLA_WARN << "Already have a MessageQueue";
  m_message_queue = new MessageQueue(m_tcp_socket, m_ss,
                                     m_message_builder->pool());

  if (m_health_checked_connection)
    OLA_WARN << "Already have a E133HealthCheckedConnection";
    m_health_checked_connection = new E133HealthCheckedConnection(
      m_message_builder,
      m_message_queue,
      ola::NewSingleCallback(
        this, &DesignatedControllerConnection::TCPConnectionUnhealthy),
      m_ss);

  // this sends a heartbeat message to indicate this is the live connection
  if (!m_health_checked_connection->Setup()) {
    OLA_WARN << "Failed to setup HealthCheckedConnection, closing TCP socket";
    delete m_health_checked_connection;
    m_health_checked_connection = NULL;
    delete m_message_queue;
    m_message_queue = NULL;
    m_tcp_socket->Close();
    delete m_tcp_socket;
    m_tcp_socket = NULL;
    return;
  }

  m_tcp_message_sender.SetMessageQueue(m_message_queue);

  if (m_incoming_tcp_transport)
    OLA_WARN << "Already have an IncomingTCPTransport";
    m_incoming_tcp_transport = new ola::plugin::e131::IncomingTCPTransport(
        &m_root_inflator, m_tcp_socket);

  m_tcp_stats->connection_events++;
  m_tcp_stats->ip_address = v4_address.Host();

  m_tcp_socket->SetOnData(
      NewCallback(this, &DesignatedControllerConnection::ReceiveTCPData));
  m_tcp_socket->SetOnClose(ola::NewSingleCallback(
      this, &DesignatedControllerConnection::TCPConnectionClosed));
  m_ss->AddReadDescriptor(m_tcp_socket);
}


/**
 * Called when there is new TCP data available
 */
void DesignatedControllerConnection::ReceiveTCPData() {
  if (m_incoming_tcp_transport) {
    if (!m_incoming_tcp_transport->Receive()) {
      OLA_WARN << "TCP STREAM IS BAD!!!";
      CloseTCPConnection();
    }
  }
}


/**
 * Called when the TCP connection goes unhealthy.
 */
void DesignatedControllerConnection::TCPConnectionUnhealthy() {
  OLA_INFO << "TCP connection went unhealthy, closing";
  m_tcp_stats->unhealthy_events++;

  CloseTCPConnection();
}


/**
 * Close and cleanup the TCP connection. This can be triggered one of three
 * ways:
 *  - remote end closes the connection
 *  - the local end decides to close the connection
 *  - the heartbeats time out
 */
void DesignatedControllerConnection::TCPConnectionClosed() {
  OLA_INFO << "TCP conection closed";

  // zero out the master's IP
  m_tcp_stats->ip_address = IPV4Address();
  m_ss->RemoveReadDescriptor(m_tcp_socket);

  // shutdown the tx side
  m_tcp_message_sender.SetMessageQueue(NULL);

  delete m_health_checked_connection;
  m_health_checked_connection = NULL;

  delete m_message_queue;
  m_message_queue = NULL;

  // shutdown the rx side
  delete m_incoming_tcp_transport;
  m_incoming_tcp_transport = NULL;

  // finally delete the socket
  m_tcp_socket->Close();
  delete m_tcp_socket;
  m_tcp_socket = NULL;
}


/**
 * Called when we receive a valid Root Layer PDU.
 */
void DesignatedControllerConnection::RLPDataReceived(const TransportHeader&) {
  if (m_health_checked_connection) {
    m_health_checked_connection->HeartbeatReceived();
  }
}


/**
 * Handle a E1.33 Status PDU on the TCP connection.
 */
void DesignatedControllerConnection::HandleStatusMessage(
    const TransportHeader &transport_header,
    const ola::plugin::e131::E133Header &e133_header,
    uint16_t status_code,
    const string &description) {
  if (status_code != ola::plugin::e131::SC_E133_ACK) {
    OLA_INFO << "Received a non-ack status code from "
             << transport_header.SourceIP() << ": " << status_code << " : "
             << description;
  }
  OLA_INFO << "Controller has ack'ed " << e133_header.Sequence();
  m_tcp_message_sender.Acknowledge(e133_header.Sequence());
}
