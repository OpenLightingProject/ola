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
#include <ola/network/HealthCheckedConnection.h>
#include <ola/network/SelectServerInterface.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMHelper.h>

#include <memory>
#include <string>
#include <vector>

#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/DMPAddress.h"
#include "plugins/e131/e131/DMPPDU.h"
#include "plugins/e131/e131/E133Header.h"
#include "plugins/e131/e131/UDPTransport.h"

#include "tools/e133/E133Device.h"
#include "tools/e133/E133Endpoint.h"
#include "tools/e133/E133HealthCheckedConnection.h";

using ola::NewCallback;
using ola::network::HealthCheckedConnection;
using ola::plugin::e131::DMPAddressData;
using ola::plugin::e131::TwoByteRangeDMPAddress;
using std::auto_ptr;
using std::string;


E133Device::E133Device(ola::network::SelectServerInterface *ss,
                       const ola::network::IPV4Address &ip_address)
    : m_cid(ola::plugin::e131::CID::Generate()),
      m_health_check_interval(2, 0),
      m_tcp_descriptor(NULL),
      m_health_checked_connection(NULL),
      m_ss(ss),
      m_ip_address(ip_address),
      m_incoming_udp_transport(&m_udp_socket, &m_root_inflator),
      m_outgoing_udp_transport(&m_udp_socket),
      m_root_sender(m_cid),
      m_e133_sender(&m_root_sender) {
}


E133Device::~E133Device() {
  // de-register all endpoints with the DMP layer
  endpoint_map::iterator iter = m_endpoint_map.begin();
  for (; iter != m_endpoint_map.end(); ++iter) {
    m_dmp_inflator.RemoveRDMHandler(iter->first);
  }
  m_endpoint_map.clear();
}


bool E133Device::Init() {
  OLA_INFO << "Attempting to start E1.33 at " << m_ip_address;

  // setup the TCP socket
  m_tcp_socket.SetOnAccept(NewCallback(this, &E133Device::NewTCPConnection));
  bool listen_ok = m_tcp_socket.Listen(m_ip_address,
                                       ola::plugin::e131::ACN_PORT);
  if (!listen_ok) {
    m_tcp_socket.Close();
    return false;
  }

  // setup the UDP socket
  if (!m_udp_socket.Init()) {
    m_tcp_socket.Close();
    return false;
  }

  if (!m_udp_socket.Bind(ola::plugin::e131::ACN_PORT)) {
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
 * Register a E133UniverseEndpoint
 * @param endpoint_id the endpoint index
 * @param endpoint E133UniverseEndpoint to register
 * @return true if the registration succeeded, false otherwise.
 */
bool E133Device::RegisterEndpoint(uint16_t endpoint_id,
                                  E133Endpoint *endpoint) {
  if (!endpoint_id) {
    OLA_WARN << "Can't unregister the root endpoint";
  }

  endpoint_map::iterator iter = m_endpoint_map.find(endpoint_id);
  if (iter == m_endpoint_map.end()) {
    m_endpoint_map[endpoint_id] = endpoint;
    m_dmp_inflator.SetRDMHandler(
        endpoint_id,
        NewCallback(this, &E133Device::EndpointRequest, endpoint_id));
    return true;
  }
  return false;
}


/**
 * Unregister a E133Endpoint
 * @param endpoint_id the index of the endpont to de-register
 */
void E133Device::UnRegisterEndpoint(uint16_t endpoint_id) {
  endpoint_map::iterator iter = m_endpoint_map.find(endpoint_id);
  if (iter != m_endpoint_map.end()) {
    m_dmp_inflator.RemoveRDMHandler(endpoint_id);
    m_endpoint_map.erase(iter);
  }
}


/**
 * Called when we get a new TCP connection.
 */
void E133Device::NewTCPConnection(
    ola::network::ConnectedDescriptor *descriptor) {
  OLA_INFO << "New TCP connection to E1.33 Node";

  if (m_health_checked_connection) {
    OLA_WARN << "Already got a TCP connection open, closing this one";
    descriptor->Close();
    delete descriptor;
    return;
  }

  m_health_checked_connection = new
    E133HealthCheckedConnection(
        &m_e133_sender,
        ola::NewSingleCallback(this, &E133Device::TCPConnectionUnhealthy),
        descriptor,
        m_ss,
        m_health_check_interval);
  if (!m_health_checked_connection->Setup()) {
    OLA_WARN <<
      "Failed to setup HealthCheckedConnection, closing TCP connection";
    delete m_health_checked_connection;
    m_health_checked_connection = NULL;
    descriptor->Close();
    delete descriptor;
    return;
  }

  // send a heartbeat message to indicate this is the live connection
  m_health_checked_connection->SendHeartbeat();
  m_tcp_descriptor = descriptor;
  m_ss->AddReadDescriptor(descriptor);
}


/**
 * Called when the TCP connection goes unhealthy.
 */
void E133Device::TCPConnectionUnhealthy() {
  OLA_INFO << "TCP connection went unhealthy, closing";

  delete m_health_checked_connection;
  m_health_checked_connection = NULL;

  m_ss->RemoveReadDescriptor(m_tcp_descriptor);
  delete m_tcp_descriptor;
  m_tcp_descriptor = NULL;
}


/**
 * Handle requests to an endpoint.
 */
void E133Device::EndpointRequest(
    uint16_t endpoint_id,
    const ola::plugin::e131::TransportHeader &transport_header,
    const ola::plugin::e131::E133Header &e133_header,
    const std::string &raw_request) {
  OLA_INFO << "Got request for to endpoint " << endpoint_id << "from " <<
    transport_header.SourceIP();

  endpoint_map::iterator iter = m_endpoint_map.find(endpoint_id);
  if (iter == m_endpoint_map.end()) {
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

  iter->second->SendRDMRequest(
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
    const std::vector<std::string> &packets) {
  auto_ptr<const ola::rdm::RDMResponse> response(response_ptr);

  if (response_code != ola::rdm::RDM_COMPLETED_OK) {
    OLA_WARN << "E1.33 request failed with code " <<
      ola::rdm::ResponseCodeToString(response_code) <<
      ", dropping request";
    return;
  }

  OLA_INFO << "rdm size is " << response->Size();

  // TODO(simon): handle the ack overflow case here
  // For now we just send back one packet.
  unsigned int actual_size = response->Size();
  uint8_t *rdm_data = new uint8_t[actual_size + 1];
  rdm_data[0] = ola::rdm::RDMCommand::START_CODE;

  if (!response->Pack(rdm_data + 1, &actual_size)) {
    OLA_WARN << "Failed to pack RDM response, aborting send";
    delete[] rdm_data;
    return;
  }
  unsigned int rdm_data_size = actual_size + 1;

  ola::plugin::e131::TwoByteRangeDMPAddress range_addr(0,
                                                       1,
                                                       rdm_data_size);
  DMPAddressData<
    TwoByteRangeDMPAddress> range_chunk(
        &range_addr,
        rdm_data,
        rdm_data_size);
  std::vector<DMPAddressData<TwoByteRangeDMPAddress> > ranged_chunks;
  ranged_chunks.push_back(range_chunk);
  const ola::plugin::e131::DMPPDU *pdu =
    ola::plugin::e131::NewRangeDMPSetProperty<uint16_t>(
        true,
        false,
        ranged_chunks);

  // TODO(simon): support timeouts here
  ola::plugin::e131::E133Header header(
      "foo bar",
      sequence_number,
      endpoint_id,
      false,  // rx_ack
      false);  // timeout

  ola::plugin::e131::OutgoingUDPTransport transport(&m_outgoing_udp_transport,
                                                    src_ip,
                                                    src_port);
  bool result = m_e133_sender.SendDMP(header, pdu, &transport);
  if (!result)
    OLA_WARN << "Failed to send E1.33 response";

  // send response back to src ip:port with correct seq #
  delete[] rdm_data;
  delete pdu;

  (void) packets;
}
