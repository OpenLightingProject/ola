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

using ola::rdm::RDMCallback;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using std::auto_ptr;
using std::vector;


typedef std::vector<std::string> RDMPackets;

/**
 * Create a new RootEndpoint
 */
RootEndpoint::RootEndpoint(const ola::rdm::UID &uid,
                           const class EndpointManager *endpoint_manager)
    : E133EndpointInterface(),
      m_uid(uid),
      m_endpoint_manager(endpoint_manager) {
}


/**
 * Handle a RDM request for the Root Endpoint
 * @param request_ptr the RDMRequest object
 * @param on_complete the callback to run when we've handled this request.
 */
void RootEndpoint::SendRDMRequest(const RDMRequest *request_ptr,
                                  RDMCallback *on_complete) {
  auto_ptr<const RDMRequest> request(request_ptr);

  if (request->DestinationUID() != m_uid) {
    OLA_WARN << "Got a request to the root endpoint for the incorrect UID." <<
      "Expected " << m_uid << ", got " << request->DestinationUID();
    RDMPackets packets;
    on_complete->Run(ola::rdm::RDM_UNKNOWN_UID, NULL, packets);
    return;
  }

  OLA_INFO << "Received Request for root endpoint";
  switch (request->ParamId()) {
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
 * Handle PID_ENDPOINT_LIST.
 */
void RootEndpoint::HandleEndpointList(const ola::rdm::RDMRequest *request,
                                      ola::rdm::RDMCallback *on_complete) {
  if (!CheckForBroadcastSubdeviceOrData(request, on_complete))
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
  // TODO(simon): add me
  HandleUnknownPID(request, on_complete);
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
  // TODO(simon): add me
  HandleUnknownPID(request, on_complete);

  /*
   * remote_v4_address
   * connection_unhealthy_events
   * connection_establishments
   */
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
 * Check for the following:
 *   - the callback was non-null
 *   - broadcast request
 *   - request with a sub device set
 *   - request with data
 * And return the correct NACK reason
 * @returns true is this request was ok, false if we nack'ed it
 */
bool RootEndpoint::CheckForBroadcastSubdeviceOrData(
    const RDMRequest *request,
    RDMCallback *callback) {
  if (request->DestinationUID().IsBroadcast()) {
    RDMPackets packets;
    callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
    return false;
  }

  RDMResponse *response = NULL;
  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    response = NackWithReason(request, ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS);
  } else if (request->SubDevice()) {
    response = NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  } else if (request->ParamDataSize()) {
    response = NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
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
