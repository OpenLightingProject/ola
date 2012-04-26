/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * E133Device.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/HealthCheckedConnection.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMHelper.h>

#include <memory>
#include <string>
#include <vector>

#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/E133Header.h"
#include "plugins/e131/e131/E133PDU.h"
#include "plugins/e131/e131/RDMPDU.h"
#include "plugins/e131/e131/RDMInflator.h"
#include "plugins/e131/e131/UDPTransport.h"

#include "tools/e133/E133Device.h"
#include "tools/e133/E133Endpoint.h"
#include "tools/e133/E133HealthCheckedConnection.h"
#include "tools/e133/EndpointManager.h"
#include "tools/e133/TCPConnectionStats.h"

using ola::NewCallback;
using ola::network::HealthCheckedConnection;
using ola::network::IPV4Address;
using ola::plugin::e131::RDMPDU;
using std::auto_ptr;
using std::string;
using std::vector;


E133Device::E133Device(ola::io::SelectServerInterface *ss,
                       const ola::network::IPV4Address &ip_address,
                       EndpointManager *endpoint_manager,
                       TCPConnectionStats *tcp_stats)
    : m_endpoint_manager(endpoint_manager),
      m_register_endpoint_callback(NULL),
      m_unregister_endpoint_callback(NULL),
      m_root_endpoint(NULL),
      m_tcp_stats(tcp_stats),
      m_cid(ola::plugin::e131::CID::Generate()),
      m_tcp_descriptor(NULL),
      m_outgoing_tcp_transport(NULL),
      m_health_checked_connection(NULL),
      m_ss(ss),
      m_ip_address(ip_address),
      m_root_inflator(NewCallback(this, &E133Device::RLPDataReceived)),
      m_incoming_udp_transport(&m_udp_socket, &m_root_inflator),
      m_outgoing_udp_transport(&m_udp_socket),
      m_incoming_tcp_transport(NULL),
      m_root_sender(m_cid),
      m_e133_sender(&m_root_sender, string("OLA Device")) {

  m_root_inflator.AddInflator(&m_e133_inflator);
  m_e133_inflator.AddInflator(&m_rdm_inflator);

  m_register_endpoint_callback.reset(NewCallback(
      this,
      &E133Device::RegisterEndpoint));
  m_unregister_endpoint_callback.reset(NewCallback(
      this,
      &E133Device::UnRegisterEndpoint));
  m_endpoint_manager->RegisterNotification(
      EndpointManager::ADD,
      m_register_endpoint_callback.get());
  m_endpoint_manager->RegisterNotification(
      EndpointManager::REMOVE,
      m_unregister_endpoint_callback.get());
}


E133Device::~E133Device() {
  vector<uint16_t> endpoints;
  m_endpoint_manager->EndpointIDs(&endpoints);
  if (endpoints.size()) {
    OLA_WARN << "Some endpoints weren't removed correctly";
    vector<uint16_t>::iterator iter = endpoints.begin();
    for (; iter != endpoints.end(); ++iter) {
      m_rdm_inflator.RemoveRDMHandler(*iter);
    }
  }

  m_endpoint_manager->UnRegisterNotification(
      m_register_endpoint_callback.get());
  m_endpoint_manager->UnRegisterNotification(
      m_unregister_endpoint_callback.get());
}


/**
 * Set the Root Endpoint, ownership is not transferred
 */
void E133Device::SetRootEndpoint(E133EndpointInterface *endpoint) {
  m_root_endpoint = endpoint;
  // register the root enpoint
  m_rdm_inflator.SetRDMHandler(
      0,
      NewCallback(this,
                  &E133Device::EndpointRequest,
                  static_cast<uint16_t>(0)));
}


/**
 * Init the device.
 */
bool E133Device::Init() {
  OLA_INFO << "Attempting to start E1.33 device at " << m_ip_address;

  // setup the TCP socket
  m_tcp_socket.SetOnAccept(NewCallback(this, &E133Device::NewTCPConnection));
  bool listen_ok = m_tcp_socket.Listen(m_ip_address,
                                       ola::plugin::e131::E133_PORT);
  if (!listen_ok) {
    m_tcp_socket.Close();
    return false;
  }

  // setup the UDP socket
  if (!m_udp_socket.Init()) {
    m_tcp_socket.Close();
    return false;
  }

  if (!m_udp_socket.Bind(ola::plugin::e131::E133_PORT)) {
    m_tcp_socket.Close();
    return false;
  }

  m_udp_socket.SetOnData(
        NewCallback(&m_incoming_udp_transport,
                    &ola::plugin::e131::IncomingUDPTransport::Receive));

  // add both to the Select Server
  m_ss->AddReadDescriptor(&m_udp_socket);
  m_ss->AddReadDescriptor(&m_tcp_socket);
  return true;
}


/**
 * Send an unsolicated RDM message on the TCP channel.
 * @param command the RDM command to send, ownership is transferred.
 */
void E133Device::SendStatusMessage(const ola::rdm::RDMCommand *command) {
  const RDMPDU *rdm_pdu = new RDMPDU(command);

  bool ok = m_e133_sender.SendReliably(
      ola::plugin::e131::RDMInflator::RDM_VECTOR,
      ROOT_E133_ENDPOINT,
      rdm_pdu);
  if (!ok)
    delete rdm_pdu;
}


/**
 * Close the master's TCP connection.
 * @return, true if there was a connection to close, false otherwise
 */
bool E133Device::CloseTCPConnection() {
  if (!m_tcp_descriptor)
    return false;

  delete m_outgoing_tcp_transport;
  m_outgoing_tcp_transport = NULL;

  m_ss->RemoveReadDescriptor(m_tcp_descriptor);
  m_tcp_descriptor->Close();

  ola::io::ConnectedDescriptor::OnCloseCallback *callback =
    m_tcp_descriptor->TransferOnClose();
  callback->Run();
  return true;
}


/**
 * Called when we get a new TCP connection.
 */
void E133Device::NewTCPConnection(
    ola::network::TcpSocket *descriptor) {
  IPV4Address ip_address;
  uint16_t port;
  if (descriptor->GetPeer(&ip_address, &port))
    OLA_INFO << "New TCP connection from " << ip_address << ":" << port;
  else
    OLA_WARN << "New TCP connection but failed to determine peer address";

  if (m_health_checked_connection) {
    OLA_WARN << "Already got a TCP connection open, closing this one";
    descriptor->Close();
    delete descriptor;
    return;
  }

  if (m_outgoing_tcp_transport)
    OLA_WARN << "Already have a OutgoingTCPTransport";

  m_outgoing_tcp_transport = new
    ola::plugin::e131::OutgoingStreamTransport(descriptor);
  m_e133_sender.SetTransport(m_outgoing_tcp_transport);

  if (m_tcp_stats) {
    m_tcp_stats->connection_events++;
    m_tcp_stats->ip_address = ip_address;
  }

  descriptor->SetOnClose(
      ola::NewSingleCallback(this, &E133Device::TCPConnectionClosed));

  m_health_checked_connection = new
    E133HealthCheckedConnection(
        m_outgoing_tcp_transport,
        &m_root_sender,
        ola::NewSingleCallback(this, &E133Device::TCPConnectionUnhealthy),
        m_ss);
  if (!m_health_checked_connection->Setup()) {
    OLA_WARN <<
      "Failed to setup HealthCheckedConnection, closing TCP connection";
    delete m_health_checked_connection;
    m_health_checked_connection = NULL;
    descriptor->Close();
    delete descriptor;
    return;
  }

  m_incoming_tcp_transport = new ola::plugin::e131::IncomingTCPTransport(
      &m_root_inflator,
      descriptor);

  // send a heartbeat message to indicate this is the live connection
  m_health_checked_connection->SendHeartbeat();
  m_tcp_descriptor = descriptor;

  descriptor->SetOnData(
      NewCallback(this,
                  &E133Device::ReceiveTCPData,
                  m_incoming_tcp_transport));
  m_ss->AddReadDescriptor(descriptor);
}


/**
 * Called when there is new TCP data available
 */
void E133Device::ReceiveTCPData(
    ola::plugin::e131::IncomingTCPTransport *transport) {
  bool ok = transport->Receive();
  if (!ok) {
    OLA_WARN << "TCP STREAM IS BAD!!!";
  }
}


/**
 * Called when the TCP connection goes unhealthy.
 */
void E133Device::TCPConnectionUnhealthy() {
  OLA_INFO << "TCP connection went unhealthy, closing";
  if (m_tcp_stats)
    m_tcp_stats->unhealthy_events++;

  CloseTCPConnection();
}


/**
 * Called when the TCP connection is closed
 */
void E133Device::TCPConnectionClosed() {
  OLA_INFO << "TCP conection closed";
  m_tcp_stats->ip_address = IPV4Address();

  delete m_health_checked_connection;
  m_health_checked_connection = NULL;

  delete m_incoming_tcp_transport;
  m_incoming_tcp_transport = NULL;

  delete m_tcp_descriptor;
  m_tcp_descriptor = NULL;
}


/**
 * Called when we receive E1.33 data. If this arrived over TCP we notify the
 * health checked connection.
 */
void E133Device::RLPDataReceived(
    const ola::plugin::e131::TransportHeader &header) {
  OLA_INFO << "Got Root PDU from " << header.SourceIP();
  if (header.Transport() == ola::plugin::e131::TransportHeader::TCP &&
      m_health_checked_connection) {
    m_health_checked_connection->HeartbeatReceived();
  }
}


/**
 * Caled when new endpoints are added
 */
void E133Device::RegisterEndpoint(uint16_t endpoint_id) {
  OLA_INFO << "Endpoint " << endpoint_id << " has been added";
  m_rdm_inflator.SetRDMHandler(
      endpoint_id,
      NewCallback(this, &E133Device::EndpointRequest, endpoint_id));
}


/**
 * Called when endpoints are removed
 */
void E133Device::UnRegisterEndpoint(uint16_t endpoint_id) {
  OLA_INFO << "Endpoint " << endpoint_id << " has been removed";
  m_rdm_inflator.RemoveRDMHandler(endpoint_id);
}


/**
 * Handle requests to an endpoint.
 */
void E133Device::EndpointRequest(
    uint16_t endpoint_id,
    const ola::plugin::e131::TransportHeader &transport_header,
    const ola::plugin::e131::E133Header &e133_header,
    const std::string &raw_request) {
  OLA_INFO << "Got request for to endpoint " << endpoint_id << " from " <<
    transport_header.SourceIP();

  E133EndpointInterface *endpoint = NULL;
  if (endpoint_id)
    endpoint = m_endpoint_manager->GetEndpoint(endpoint_id);
  else
    endpoint = m_root_endpoint;

  if (!endpoint) {
    OLA_INFO << "Request to endpoint " << endpoint_id <<
      " but no Endpoint has been registered, this is a bug!";
    return;
  }

  // attempt to unpack as a request
  const ola::rdm::RDMRequest *request = ola::rdm::RDMRequest::InflateFromData(
    reinterpret_cast<const uint8_t*>(raw_request.data()),
    raw_request.size());

  if (!request) {
    OLA_WARN << "Failed to unpack E1.33 RDM message, ignoring request.";
    return;
  }

  endpoint->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &E133Device::EndpointRequestComplete,
                             transport_header.SourceIP(),
                             transport_header.SourcePort(),
                             e133_header.Sequence(),
                             endpoint_id));
}


/**
 * Handle a completed RDM request.
 */
void E133Device::EndpointRequestComplete(
    ola::network::IPV4Address src_ip,
    uint16_t src_port,
    uint32_t sequence_number,
    uint16_t endpoint_id,
    ola::rdm::rdm_response_code response_code,
    const ola::rdm::RDMResponse *response_ptr,
    const std::vector<std::string>&) {
  /*
   * TODO(simon): map internal status codes to E1.33 codes once these are added
   * to the spec.
   *  - RDM_UNKNOWN_UID -> timeout
   */
  if (response_code != ola::rdm::RDM_COMPLETED_OK) {
    if (response_code != ola::rdm::RDM_WAS_BROADCAST)
      OLA_WARN << "E1.33 request failed with code " <<
        ola::rdm::ResponseCodeToString(response_code) <<
        ", dropping request";
    delete response_ptr;
    return;
  }

  // rdm_pdu now owns the response_ptr
  const RDMPDU rdm_pdu(response_ptr);

  ola::plugin::e131::E133Header header(
      "foo bar",
      sequence_number,
      endpoint_id,
      false);  // rx_ack

  ola::plugin::e131::E133PDU pdu(ola::plugin::e131::RDMInflator::RDM_VECTOR,
                                 header,
                                 &rdm_pdu);
  ola::plugin::e131::OutgoingUDPTransport transport(&m_outgoing_udp_transport,
                                                    src_ip,
                                                    src_port);
  bool result = m_root_sender.SendPDU(
      ola::plugin::e131::E133Inflator::E133_VECTOR,
      pdu,
      &transport);
  if (!result)
    OLA_WARN << "Failed to send E1.33 response";
}
