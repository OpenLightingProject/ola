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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * DeviceManagerImpl.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/Logging.h>
#include <ola/acn/ACNPort.h>
#include <ola/acn/CID.h>
#include <ola/base/Macro.h>
#include <ola/e133/E133Enums.h>
#include <ola/io/NonBlockingSender.h>
#include <ola/io/SelectServer.h>
#include <ola/network/AdvancedTCPConnector.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Socket.h>
#include <ola/network/TCPSocketFactory.h>
#include <ola/stl/STLUtils.h>

#include <memory>
#include <string>
#include <vector>

#include "libs/acn/E133Inflator.h"
#include "libs/acn/E133StatusPDU.h"
#include "libs/acn/TCPTransport.h"

#include "tools/e133/DeviceManagerImpl.h"
#include "tools/e133/E133Endpoint.h"
#include "tools/e133/E133HealthCheckedConnection.h"

namespace ola {
namespace e133 {

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::STLContains;
using ola::STLFindOrNull;
using ola::TimeInterval;
using ola::acn::CID;
using ola::io::NonBlockingSender;
using ola::network::GenericSocketAddress;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPSocket;
using ola::acn::IncomingTCPTransport;

using std::auto_ptr;
using std::string;


/**
 * Holds everything we need to manage a TCP connection to a E1.33 device.
 */
class DeviceState {
 public:
    DeviceState()
      : socket(NULL),
        message_queue(NULL),
        health_checked_connection(NULL),
        in_transport(NULL),
        am_designated_controller(false) {
    }

    // The following may be NULL.
    // The socket connected to the E1.33 device
    auto_ptr<TCPSocket> socket;
    auto_ptr<NonBlockingSender> message_queue;
    // The Health Checked connection
    auto_ptr<E133HealthCheckedConnection> health_checked_connection;
    auto_ptr<IncomingTCPTransport> in_transport;

    // True if we're the designated controller.
    bool am_designated_controller;

 private:
    DISALLOW_COPY_AND_ASSIGN(DeviceState);
};


// 5 second connect() timeout
const TimeInterval DeviceManagerImpl::TCP_CONNECT_TIMEOUT(5, 0);
// retry TCP connects after 5 seconds
const TimeInterval DeviceManagerImpl::INITIAL_TCP_RETRY_DELAY(5, 0);
// we grow the retry interval to a max of 30 seconds
const TimeInterval DeviceManagerImpl::MAX_TCP_RETRY_DELAY(30, 0);


/**
 * Construct a new DeviceManagerImpl
 * @param ss a pointer to a SelectServerInterface to use
 * @param cid the CID of this controller.
 */
DeviceManagerImpl::DeviceManagerImpl(ola::io::SelectServerInterface *ss,
                             ola::e133::MessageBuilder *message_builder)
    : m_ss(ss),
      m_tcp_socket_factory(NewCallback(this, &DeviceManagerImpl::OnTCPConnect)),
      m_connector(m_ss, &m_tcp_socket_factory, TCP_CONNECT_TIMEOUT),
      m_backoff_policy(INITIAL_TCP_RETRY_DELAY, MAX_TCP_RETRY_DELAY),
      m_message_builder(message_builder),
      m_root_inflator(NewCallback(this, &DeviceManagerImpl::RLPDataReceived)) {
  m_root_inflator.AddInflator(&m_e133_inflator);
  m_e133_inflator.AddInflator(&m_rdm_inflator);
  m_rdm_inflator.SetRDMHandler(
      NewCallback(this, &DeviceManagerImpl::EndpointRequest));
}


/**
 * Clean up
 */
DeviceManagerImpl::~DeviceManagerImpl() {
  // close out all tcp sockets and free state
  ola::STLDeleteValues(&m_device_map);
}


/**
 * Set the callback to be run when RDMNet data is received from a device.
 * @param callback the RDMMessageCallback to run when data is received.
 */
void DeviceManagerImpl::SetRDMMessageCallback(RDMMessageCallback *callback) {
  m_rdm_callback.reset(callback);
}


/**
 * Set the callback to be run when we become the designated controller for a
 * device.
 */
void DeviceManagerImpl::SetAcquireDeviceCallback(
    AcquireDeviceCallback *callback) {
  m_acquire_device_cb_.reset(callback);
}


/*
 * Set the callback to be run when we lose the designated controller status for
 * a device.
 */
void DeviceManagerImpl::SetReleaseDeviceCallback(
    ReleaseDeviceCallback *callback) {
  m_release_device_cb_.reset(callback);
}


/**
 * Start maintaining a connection to this device.
 */
void DeviceManagerImpl::AddDevice(const IPV4Address &ip_address) {
  if (STLContains(m_device_map, ip_address.AsInt())) {
    return;
  }

  DeviceState *device_state = new DeviceState();
  m_device_map[ip_address.AsInt()] = device_state;

  OLA_INFO << "Adding " << ip_address << ":" << ola::acn::E133_PORT;
  // start the non-blocking connect
  m_connector.AddEndpoint(
      IPV4SocketAddress(ip_address, ola::acn::E133_PORT),
      &m_backoff_policy);
}


/**
 * Remove a device, closing the connection if we have one.
 */
void DeviceManagerImpl::RemoveDevice(const IPV4Address &ip_address) {
  DeviceMap::iterator iter = m_device_map.find(ip_address.AsInt());
  if (iter == m_device_map.end())
    return;

  // TODO(simon): implement this
  OLA_WARN << "RemoveDevice not implemented";
}


/**
 * Remove a device if there is no open connection.
 */
void DeviceManagerImpl::RemoveDeviceIfNotConnected(
    const IPV4Address &ip_address) {
  DeviceMap::iterator iter = m_device_map.find(ip_address.AsInt());
  if (iter == m_device_map.end())
    return;

  // TODO(simon): implement this
  OLA_WARN << "RemoveDevice not implemented";
}


/**
 * Populate the vector with the devices that we are the designated controller
 * for.
 */
void DeviceManagerImpl::ListManagedDevices(vector<IPV4Address> *devices) const {
  DeviceMap::const_iterator iter = m_device_map.begin();
  for (; iter != m_device_map.end(); ++iter) {
    if (iter->second->am_designated_controller)
      devices->push_back(IPV4Address(iter->first));
  }
}


/**
 * Called when a TCP socket is connected. Note that we're not the designated
 * controller at this point. That only happens if we receive data on the
 * connection.
 */
void DeviceManagerImpl::OnTCPConnect(TCPSocket *socket_ptr) {
  auto_ptr<TCPSocket> socket(socket_ptr);
  GenericSocketAddress address = socket->GetPeerAddress();
  if (address.Family() != AF_INET) {
    OLA_WARN << "Non IPv4 socket " << address;
    return;
  }
  IPV4SocketAddress v4_address = address.V4Addr();
  DeviceState *device_state = STLFindOrNull(
      m_device_map, v4_address.Host().AsInt());
  if (!device_state) {
    OLA_FATAL << "Unable to locate socket for " << v4_address;
    return;
  }

  // setup the incoming transport, we don't need to setup the outgoing one
  // until we've got confirmation that we're the designated controller.
  device_state->socket.reset(socket.release());
  device_state->in_transport.reset(new IncomingTCPTransport(&m_root_inflator,
                                                            socket_ptr));

  device_state->socket->SetOnData(
      NewCallback(this, &DeviceManagerImpl::ReceiveTCPData, v4_address.Host(),
                  device_state->in_transport.get()));
  device_state->socket->SetOnClose(
      NewSingleCallback(this, &DeviceManagerImpl::SocketClosed,
                        v4_address.Host()));
  m_ss->AddReadDescriptor(socket_ptr);

  // TODO(simon): Setup a timeout that closes this connect if we don't receive
  // anything.
}


/**
 * Receive data on a TCP connection
 */
void DeviceManagerImpl::ReceiveTCPData(IPV4Address ip_address,
                                   IncomingTCPTransport *transport) {
  if (!transport->Receive()) {
    OLA_WARN << "TCP STREAM IS BAD!!!";
    SocketClosed(ip_address);
  }
}


/**
 * Called when a connection is deemed unhealthy.
 */
void DeviceManagerImpl::SocketUnhealthy(IPV4Address ip_address) {
  OLA_INFO << "connection to " << ip_address << " went unhealthy";
  SocketClosed(ip_address);
}


/**
 * Called when a socket is closed.
 * This can mean one of two things:
 *  if we weren't the designated controller, then we lost the race.
 *  if we were the designated controller, the TCP connection was closed, or
 *  went unhealthy.
 */
void DeviceManagerImpl::SocketClosed(IPV4Address ip_address) {
  OLA_INFO << "connection to " << ip_address << " was closed";

  DeviceState *device_state = STLFindOrNull(m_device_map, ip_address.AsInt());
  if (!device_state) {
    OLA_FATAL << "Unable to locate socket for " << ip_address;
    return;
  }

  if (device_state->am_designated_controller) {
    device_state->am_designated_controller = false;
    if (m_release_device_cb_.get())
      m_release_device_cb_->Run(ip_address);

    m_connector.Disconnect(
        IPV4SocketAddress(ip_address, ola::acn::E133_PORT));
  } else {
    // we lost the race, so don't try to reconnect
    m_connector.Disconnect(
        IPV4SocketAddress(ip_address, ola::acn::E133_PORT), true);
  }

  device_state->health_checked_connection.reset();
  device_state->message_queue.reset();
  device_state->in_transport.reset();
  m_ss->RemoveReadDescriptor(device_state->socket.get());
  device_state->socket.reset();
}


/**
 * Called when we receive E1.33 data. If this arrived over TCP we notify the
 * health checked connection.
 */
void DeviceManagerImpl::RLPDataReceived(
    const ola::acn::TransportHeader &header) {
  if (header.Transport() != ola::acn::TransportHeader::TCP)
    return;
  IPV4Address src_ip = header.Source().Host();

  DeviceState *device_state = STLFindOrNull(m_device_map, src_ip.AsInt());
  if (!device_state) {
    OLA_FATAL << "Received data but unable to lookup socket for " <<
      src_ip;
    return;
  }

  // If we're already the designated controller, we just need to notify the
  // HealthChecker.
  if (device_state->am_designated_controller) {
    device_state->health_checked_connection->HeartbeatReceived();
    return;
  }

  // This is the first packet received on this connection, which is a sign
  // we're now the designated controller. Setup the HealthChecker & outgoing
  // transports.
  device_state->am_designated_controller = true;
  OLA_INFO << "Now the designated controller for " << header.Source();
  if (m_acquire_device_cb_.get())
    m_acquire_device_cb_->Run(src_ip);

  device_state->message_queue.reset(
      new NonBlockingSender(device_state->socket.get(), m_ss,
                            m_message_builder->pool()));

  E133HealthCheckedConnection *health_checked_connection =
      new E133HealthCheckedConnection(
          m_message_builder,
          device_state->message_queue.get(),
          NewSingleCallback(this, &DeviceManagerImpl::SocketUnhealthy, src_ip),
          m_ss);

  if (!health_checked_connection->Setup()) {
    OLA_WARN << "Failed to setup heartbeat controller for " << src_ip;
    SocketClosed(src_ip);
    return;
  }

  if (device_state->health_checked_connection.get())
    OLA_WARN << "pre-existing health_checked_connection for " << src_ip;
  device_state->health_checked_connection.reset(health_checked_connection);
}


/**
 * Handle a message on the TCP connection.
 */
void DeviceManagerImpl::EndpointRequest(
    const ola::acn::TransportHeader *transport_header,
    const ola::acn::E133Header *e133_header,
    const string &raw_request) {
  if (!m_rdm_callback.get())
    return;

  if (e133_header->Endpoint()) {
    OLA_WARN << "TCP message for non-0 endpoint. Endpoint = "
             << e133_header->Endpoint();
    return;
  }

  if (!m_rdm_callback->Run(transport_header->Source().Host(),
                           e133_header->Endpoint(), raw_request)) {
    // Don't send an ack
    return;
  }

  DeviceState *device_state = STLFindOrNull(
      m_device_map, transport_header->Source().Host().AsInt());
  if (!device_state) {
    OLA_WARN << "Unable to find DeviceState for " << transport_header->Source();
    return;
  }

  ola::io::IOStack packet(m_message_builder->pool());
  m_message_builder->BuildTCPE133StatusPDU(
      &packet, e133_header->Sequence(), e133_header->Endpoint(),
      ola::e133::SC_E133_ACK, "OK");
  device_state->message_queue->SendMessage(&packet);
}
}  // namespace e133
}  // namespace ola
