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
 * E133Device.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/acn/ACNVectors.h>
#include <ola/acn/CID.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/HealthCheckedConnection.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>
#include <ola/rdm/RDMCommandSerializer.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMHelper.h>

#include <memory>
#include <string>
#include <vector>

#include "libs/acn/E133Header.h"
#include "libs/acn/E133PDU.h"
#include "libs/acn/RDMPDU.h"
#include "libs/acn/RDMInflator.h"
#include "libs/acn/E133StatusInflator.h"
#include "libs/acn/UDPTransport.h"

#include "tools/e133/E133Device.h"
#include "tools/e133/E133Endpoint.h"
#include "tools/e133/E133HealthCheckedConnection.h"
#include "tools/e133/EndpointManager.h"
#include "tools/e133/TCPConnectionStats.h"

using ola::NewCallback;
using ola::io::IOStack;
using ola::network::HealthCheckedConnection;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::acn::RDMPDU;
using std::auto_ptr;
using std::string;
using std::vector;

// TODO(simon): At some point move this to a common E1.33 library.
ola::e133::E133StatusCode RDMStatusCodeToE133Status(
    ola::rdm::RDMStatusCode status_code) {
  switch (status_code) {
    case ola::rdm::RDM_COMPLETED_OK:
      return ola::e133::SC_E133_ACK;
    case ola::rdm::RDM_WAS_BROADCAST:
      return ola::e133::SC_E133_BROADCAST_COMPLETE;
    case ola::rdm::RDM_FAILED_TO_SEND:
    case ola::rdm::RDM_TIMEOUT:
      return ola::e133::SC_E133_RDM_TIMEOUT;
    case ola::rdm::RDM_UNKNOWN_UID:
      return ola::e133::SC_E133_UNKNOWN_UID;
    case ola::rdm::RDM_INVALID_RESPONSE:
    case ola::rdm::RDM_CHECKSUM_INCORRECT:
    case ola::rdm::RDM_TRANSACTION_MISMATCH:
    case ola::rdm::RDM_SUB_DEVICE_MISMATCH:
    case ola::rdm::RDM_SRC_UID_MISMATCH:
    case ola::rdm::RDM_DEST_UID_MISMATCH:
    case ola::rdm::RDM_WRONG_SUB_START_CODE:
    case ola::rdm::RDM_PACKET_TOO_SHORT:
    case ola::rdm::RDM_PACKET_LENGTH_MISMATCH:
    case ola::rdm::RDM_PARAM_LENGTH_MISMATCH:
    case ola::rdm::RDM_INVALID_COMMAND_CLASS:
    case ola::rdm::RDM_COMMAND_CLASS_MISMATCH:
    case ola::rdm::RDM_INVALID_RESPONSE_TYPE:
    case ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED:
    case ola::rdm::RDM_DUB_RESPONSE:
      return ola::e133::SC_E133_RDM_INVALID_RESPONSE;
  }
  return ola::e133::SC_E133_RDM_INVALID_RESPONSE;
}

E133Device::E133Device(ola::io::SelectServerInterface *ss,
                       const ola::acn::CID &cid,
                       const ola::network::IPV4Address &ip_address,
                       EndpointManager *endpoint_manager)
    : m_ss(ss),
      m_ip_address(ip_address),
      m_message_builder(cid, "OLA Device"),
      m_endpoint_manager(endpoint_manager),
      m_root_endpoint(NULL),
      m_root_rdm_device(NULL),
      m_incoming_udp_transport(&m_udp_socket, &m_root_inflator) {
  m_root_inflator.AddInflator(&m_e133_inflator);
  m_e133_inflator.AddInflator(&m_rdm_inflator);
  m_e133_inflator.AddInflator(&m_rdm_inflator);

  m_rdm_inflator.SetRDMHandler(
      NewCallback(this, &E133Device::EndpointRequest));
}

E133Device::~E133Device() {
  vector<uint16_t> endpoints;
  m_endpoint_manager->EndpointIDs(&endpoints);
  m_rdm_inflator.SetRDMHandler(NULL);
}

/**
 * Set the Root Endpoint, ownership is not transferred
 */
void E133Device::SetRootEndpoint(E133EndpointInterface *endpoint) {
  m_root_endpoint = endpoint;
}


/**
 * Init the device.
 */
bool E133Device::Init() {
  if (m_controller_connection.get()) {
    OLA_WARN << "Init already performed";
    return false;
  }

  OLA_INFO << "Attempting to start E1.33 device at " << m_ip_address;

  m_controller_connection.reset(new DesignatedControllerConnection(
        m_ss, m_ip_address, &m_message_builder, &m_tcp_stats));

  if (!m_controller_connection->Init()) {
    m_controller_connection.reset();
    return false;
  }

  // setup the UDP socket
  if (!m_udp_socket.Init()) {
    m_controller_connection.reset();
    return false;
  }

  if (!m_udp_socket.Bind(IPV4SocketAddress(IPV4Address::WildCard(),
                                           ola::acn::E133_PORT))) {
    m_controller_connection.reset();
    return false;
  }

  m_udp_socket.SetOnData(
        NewCallback(&m_incoming_udp_transport,
                    &ola::acn::IncomingUDPTransport::Receive));

  m_ss->AddReadDescriptor(&m_udp_socket);
  return true;
}


/**
 * Return the TCPConnectionStats.
 */
TCPConnectionStats* E133Device::GetTCPStats() {
  return &m_tcp_stats;
}


/**
 * Send an unsolicated RDM message on the TCP channel.
 * @param command the RDM command to send, ownership is transferred.
 */
void E133Device::SendStatusMessage(const ola::rdm::RDMResponse *response) {
  if (m_controller_connection.get()) {
    m_controller_connection->SendStatusMessage(ROOT_E133_ENDPOINT, response);
  } else {
    OLA_WARN << "Init has not been called";
  }
}


/**
 * Force close the designated controller's TCP connection.
 * @return, true if there was a connection to close, false otherwise.
 */
bool E133Device::CloseTCPConnection() {
  if (m_controller_connection.get()) {
    return m_controller_connection->CloseTCPConnection();
  } else {
    return false;
  }
}


/**
 * Handle requests to an endpoint.
 */
void E133Device::EndpointRequest(
    const ola::acn::TransportHeader *transport_header,
    const ola::acn::E133Header *e133_header,
    const string &raw_request) {
  IPV4SocketAddress target = transport_header->Source();
  uint16_t endpoint_id = e133_header->Endpoint();
  OLA_INFO << "Got request for endpoint " << endpoint_id << " from " << target;

  E133EndpointInterface *endpoint = NULL;
  if (endpoint_id)
    endpoint = m_endpoint_manager->GetEndpoint(endpoint_id);
  else
    endpoint = m_root_endpoint;

  if (!endpoint) {
    OLA_INFO << "Request to non-existent endpoint " << endpoint_id;
    SendStatusMessage(target, e133_header->Sequence(), endpoint_id,
                      ola::e133::SC_E133_NONEXISTENT_ENDPOINT,
                      "No such endpoint");
    return;
  }

  // attempt to unpack as a request
  ola::rdm::RDMRequest *request = ola::rdm::RDMRequest::InflateFromData(
    reinterpret_cast<const uint8_t*>(raw_request.data()),
    raw_request.size());

  if (!request) {
    OLA_WARN << "Failed to unpack E1.33 RDM message, ignoring request.";
    // There is no way to return 'invalid request' so pretend this is a timeout
    // but give a descriptive error msg.
    SendStatusMessage(target, e133_header->Sequence(), endpoint_id,
                      ola::e133::SC_E133_RDM_TIMEOUT,
                     "Invalid RDM request");
    return;
  }

  endpoint->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &E133Device::EndpointRequestComplete,
                             target,
                             e133_header->Sequence(),
                             endpoint_id));
}


/**
 * Handle a completed RDM request.
 */
void E133Device::EndpointRequestComplete(
    ola::network::IPV4SocketAddress target,
    uint32_t sequence_number,
    uint16_t endpoint_id,
    ola::rdm::RDMReply *reply) {
  if (reply->StatusCode() != ola::rdm::RDM_COMPLETED_OK) {
    string description = ola::rdm::ResponseCodeToString(reply->StatusCode());
    ola::e133::E133StatusCode e133_status_code = RDMStatusCodeToE133Status(
        reply->StatusCode());
    SendStatusMessage(target, sequence_number, endpoint_id,
                      e133_status_code, description);
    return;
  }

  IOStack packet(m_message_builder.pool());
  ola::rdm::RDMCommandSerializer::Write(*reply->Response(), &packet);
  RDMPDU::PrependPDU(&packet);
  m_message_builder.BuildUDPRootE133(
      &packet, ola::acn::VECTOR_FRAMING_RDMNET, sequence_number,
      endpoint_id);

  if (!m_udp_socket.SendTo(&packet, target)) {
    OLA_WARN << "Failed to send E1.33 response to " << target;
  }
}


void E133Device::SendStatusMessage(
    const ola::network::IPV4SocketAddress target,
    uint32_t sequence_number,
    uint16_t endpoint_id,
    ola::e133::E133StatusCode status_code,
    const string &description) {
  IOStack packet(m_message_builder.pool());
  m_message_builder.BuildUDPE133StatusPDU(
      &packet, sequence_number, endpoint_id,
      status_code, description);
  if (!m_udp_socket.SendTo(&packet, target)) {
    OLA_WARN << "Failed to send E1.33 response to " << target;
  }
}
