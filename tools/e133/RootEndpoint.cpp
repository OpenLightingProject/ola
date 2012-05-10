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
 * RootEndpoint.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UID.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "tools/e133/E133Endpoint.h"
#include "tools/e133/EndpointManager.h"
#include "tools/e133/RootEndpoint.h"
#include "tools/e133/TCPConnectionStats.h"

using ola::network::HostToNetwork;
using ola::rdm::RDMCallback;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::auto_ptr;
using std::vector;


typedef std::vector<std::string> RDMPackets;

/**
 * Create a new RootEndpoint
 */
RootEndpoint::RootEndpoint(const UID &uid,
                           const class EndpointManager *endpoint_manager,
                           TCPConnectionStats *tcp_stats)
    : E133EndpointInterface(),
      m_uid(uid),
      m_endpoint_manager(endpoint_manager),
      m_tcp_stats(tcp_stats) {
}


/**
 * Handle a RDM request for the Root Endpoint
 * @param request_ptr the RDMRequest object
 * @param on_complete the callback to run when we've handled this request.
 */
void RootEndpoint::SendRDMRequest(const RDMRequest *request_ptr,
                                  RDMCallback *on_complete) {
  auto_ptr<const RDMRequest> request(request_ptr);

  const UID dst_uid = request->DestinationUID();
  bool for_us = (dst_uid == m_uid ||
                 (dst_uid.IsBroadcast() &&
                  (dst_uid.ManufacturerId() == UID::ALL_MANUFACTURERS ||
                   dst_uid.ManufacturerId() == m_uid.ManufacturerId())));

  if (!for_us) {
    OLA_WARN << "Got a request to the root endpoint for the incorrect UID." <<
      "Expected " << m_uid << ", got " << request->DestinationUID();
    RDMPackets packets;
    on_complete->Run(ola::rdm::RDM_UNKNOWN_UID, NULL, packets);
    return;
  }

  OLA_INFO << "Received Request for root endpoint: PID "<< std::hex <<
    request->ParamId();
  switch (request->ParamId()) {
    case ola::rdm::PID_SUPPORTED_PARAMETERS:
      HandleSupportedParams(request.get(), on_complete);
      break;
    case ola::rdm::PID_ENDPOINT_LIST:
      HandleEndpointList(request.get(), on_complete);
      break;
    case ola::rdm::PID_ENDPOINT_IDENTIFY:
      HandleEndpointIdentify(request.get(), on_complete);
      break;
    case ola::rdm::PID_ENDPOINT_LABEL:
      HandleEndpointLabel(request.get(), on_complete);
      break;
    case ola::rdm::PID_TCP_COMMS_STATUS:
      HandleTCPCommsStatus(request.get(), on_complete);
      break;
    default:
      HandleUnknownPID(request.get(), on_complete);
  }
}


/**
 * Handle PID_SUPPORTED_PARAMETERS
 */
void RootEndpoint::HandleSupportedParams(const RDMRequest *request,
                                         RDMCallback *on_complete) {
  if (!SanityCheckGet(request, on_complete, 0))
    return;

  uint16_t supported_params[] = {
    // add ENDPOINT_PROXIED_DEVICES here
    ola::rdm::PID_ENDPOINT_LIST,
    ola::rdm::PID_ENDPOINT_IDENTIFY,
    ola::rdm::PID_ENDPOINT_LABEL,
    ola::rdm::PID_TCP_COMMS_STATUS,
  };

  for (unsigned int i = 0; i < sizeof(supported_params) / 2; i++)
    supported_params[i] = HostToNetwork(supported_params[i]);

  RDMResponse *response = GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(supported_params),
      sizeof(supported_params));

  RunRDMCallback(on_complete, response);
}


/**
 * Handle PID_ENDPOINT_LIST.
 */
void RootEndpoint::HandleEndpointList(const ola::rdm::RDMRequest *request,
                                      ola::rdm::RDMCallback *on_complete) {
  if (!SanityCheckGet(request, on_complete, 0))
    return;

  vector<uint16_t> endpoints;
  m_endpoint_manager->EndpointIDs(&endpoints);

  // convert to network order
  std::transform(endpoints.begin(), endpoints.end(), endpoints.begin(),
                  (uint16_t(*)(uint16_t)) ola::network::HostToNetwork);

  RDMResponse *response = GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&endpoints[0]),
      sizeof(uint16_t) * endpoints.size());

  RunRDMCallback(on_complete, response);
}


/**
 * Handle PID_ENDPOINT_IDENTIFY
 */
void RootEndpoint::HandleEndpointIdentify(const ola::rdm::RDMRequest *request,
                                          ola::rdm::RDMCallback *on_complete) {
  struct endpoint_identify_message_s {
    uint16_t endpoint_number;
    uint8_t identify_mode;
  } __attribute__((packed));
  struct endpoint_identify_message_s endpoint_identify_message;

  RDMPackets packets;
  uint16_t endpoint_id;
  if (!SanityCheckGetOrSet(request,
                           on_complete,
                           sizeof(endpoint_id),
                           sizeof(endpoint_identify_message),
                           sizeof(endpoint_identify_message)))
    return;

  memcpy(reinterpret_cast<uint8_t*>(&endpoint_id),
         request->ParamData(),
         sizeof(endpoint_id));
  endpoint_id = HostToNetwork(endpoint_id);

  E133Endpoint *endpoint = m_endpoint_manager->GetEndpoint(endpoint_id);

  // endpoint not found
  if (!endpoint) {
    if (request->DestinationUID().IsBroadcast()) {
      on_complete->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
      return;
    } else {
      RDMResponse *response = NackWithReason(request,
                                             ola::rdm::NR_DATA_OUT_OF_RANGE);
      RunRDMCallback(on_complete, response);
      return;
    }
  }

  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // SET
    memcpy(reinterpret_cast<uint8_t*>(&endpoint_identify_message),
           request->ParamData(),
           sizeof(endpoint_identify_message));

    endpoint->SetIdentifyMode(endpoint_identify_message.identify_mode);

    if (request->DestinationUID().IsBroadcast()) {
      on_complete->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
      return;
    } else {
      uint16_t return_data = HostToNetwork(endpoint_id);
      RDMResponse *response = GetResponseFromData(
          request,
          reinterpret_cast<uint8_t*>(&return_data),
          sizeof(return_data));
      RunRDMCallback(on_complete, response);
      return;
    }
  } else {
    // GET
    endpoint_identify_message.endpoint_number = HostToNetwork(endpoint_id);
    endpoint_identify_message.identify_mode = endpoint->IdentifyMode();
    RDMResponse *response = GetResponseFromData(
        request,
        reinterpret_cast<uint8_t*>(&endpoint_identify_message),
        sizeof(endpoint_identify_message));

    RunRDMCallback(on_complete, response);
  }
}


/**
 * Handle PID_ENDPOINT_LABEL
 */
void RootEndpoint::HandleEndpointLabel(const ola::rdm::RDMRequest *request,
                                       ola::rdm::RDMCallback *on_complete) {
  // TODO(simon): add me
  HandleUnknownPID(request, on_complete);
}


/**
 * Handle PID_TCP_COMMS_STATUS
 */
void RootEndpoint::HandleTCPCommsStatus(const RDMRequest *request,
                                        RDMCallback *on_complete) {
  struct tcp_stats_message_s {
    uint32_t ip_address;
    uint16_t unhealthy_events;
    uint16_t connection_events;
  } __attribute__((packed));
  struct tcp_stats_message_s tcp_stats_message;

  RDMPackets packets;
  if (!SanityCheckGetOrSet(request,
                           on_complete,
                           0,
                           0,
                           0))
    return;

  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // A SET message resets the counters
    m_tcp_stats->unhealthy_events = 0;
    m_tcp_stats->connection_events = 0;

    if (request->DestinationUID().IsBroadcast()) {
      on_complete->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
      return;
    } else {
      RDMResponse *response = GetResponseFromData(request, NULL, 0);
      RunRDMCallback(on_complete, response);
    }
  } else {
    // GET
    tcp_stats_message.ip_address = m_tcp_stats->ip_address.AsInt();
    tcp_stats_message.unhealthy_events =
      HostToNetwork(m_tcp_stats->unhealthy_events);
    tcp_stats_message.connection_events =
      HostToNetwork(m_tcp_stats->connection_events);

    RDMResponse *response = GetResponseFromData(
        request,
        reinterpret_cast<uint8_t*>(&tcp_stats_message),
        sizeof(tcp_stats_message));

    RunRDMCallback(on_complete, response);
  }
}


/**
 * Response with a NR_UNKNOWN_PID.
 */
void RootEndpoint::HandleUnknownPID(const RDMRequest *request,
                                    RDMCallback *on_complete) {
  RDMPackets packets;
  if (request->DestinationUID().IsBroadcast()) {
    on_complete->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
  } else {
    RunRDMCallback(on_complete,
                   NackWithReason(request, ola::rdm::NR_UNKNOWN_PID));
  }
}


/**
 * Perform sanity checks a GET only request, sending the correct NACK if any
 * checks fail.
 * @param get_length, NACK if the data length doesn't match
 * @returns true is this request was ok, false if we nack'ed it
 */
bool RootEndpoint::SanityCheckGet(const RDMRequest *request,
                                  RDMCallback *callback,
                                  unsigned int get_length) {
  if (request->DestinationUID().IsBroadcast()) {
    // don't take any action for broadcast GETs
    RDMPackets packets;
    callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
    return false;
  }

  RDMResponse *response = NULL;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    response = NackWithReason(request, ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS);
  } else if (request->SubDevice()) {
    response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->ParamDataSize() != get_length) {
    response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  if (response) {
    RunRDMCallback(callback, response);
    return false;
  }
  return true;
}


/**
 * Perform sanity checks for a GET/SET request, sending the correct NACK if any
 * checks fail
 * @param get_length, NACK if the data length doesn't match
 * @param min_set_length, NACK if the data length doesn't match
 * @param max_set_length, NACK if the data length doesn't match
 * @returns true is this request was ok, false if we nack'ed it
 */
bool RootEndpoint::SanityCheckGetOrSet(
    const RDMRequest *request,
    RDMCallback *callback,
    unsigned int get_length,
    unsigned int min_set_length,
    unsigned int max_set_length) {
  RDMPackets packets;
  RDMResponse *response = NULL;

  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // SET
    if (request->SubDevice()) {
      response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
    } else if (request->ParamDataSize() < min_set_length ||
               request->ParamDataSize() > max_set_length) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    }

    if (response && request->DestinationUID().IsBroadcast()) {
      callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
      return false;
    }
  } else {
    // GET
    // don't take any action for broadcast GETs
    if (request->DestinationUID().IsBroadcast()) {
      callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
      return false;
    }

    if (request->SubDevice()) {
      response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
    } else if (request->ParamDataSize() != get_length) {
      response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
    }
  }

  if (response) {
    RunRDMCallback(callback, response);
    return false;
  }
  return true;
}


/**
 * Run the RDM callback with a response. This takes care of creating the fake
 * raw data.
 */
void RootEndpoint::RunRDMCallback(RDMCallback *callback,
                                  RDMResponse *response) {
  string raw_response;
  response->Pack(&raw_response);
  RDMPackets packets;
  packets.push_back(raw_response);
  callback->Run(ola::rdm::RDM_COMPLETED_OK, response, packets);
}
