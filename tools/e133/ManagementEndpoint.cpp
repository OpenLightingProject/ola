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
 * ManagementEndpoint.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/ResponderHelper.h>
#include <ola/rdm/UID.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "tools/e133/E133Endpoint.h"
#include "tools/e133/EndpointManager.h"
#include "tools/e133/ManagementEndpoint.h"
#include "tools/e133/TCPConnectionStats.h"

using ola::network::HostToNetwork;
using ola::rdm::NR_ENDPOINT_NUMBER_INVALID;
using ola::rdm::NR_FORMAT_ERROR;
using ola::rdm::NR_UNKNOWN_PID;
using ola::rdm::RDMCallback;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::ResponderHelper;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::auto_ptr;
using std::vector;

ManagementEndpoint::RDMOps *ManagementEndpoint::RDMOps::instance = NULL;

const ola::rdm::ResponderOps<ManagementEndpoint>::ParamHandler
    ManagementEndpoint::PARAM_HANDLERS[] = {
  { ola::rdm::PID_ENDPOINT_LIST,
    &ManagementEndpoint::GetEndpointList,
    NULL},
  { ola::rdm::PID_ENDPOINT_LIST_CHANGE,
    &ManagementEndpoint::GetEndpointListChange,
    NULL},
  { ola::rdm::PID_IDENTIFY_ENDPOINT,
    &ManagementEndpoint::GetIdentifyEndpoint,
    &ManagementEndpoint::SetIdentifyEndpoint},
  { ola::rdm::PID_ENDPOINT_TO_UNIVERSE,
    &ManagementEndpoint::GetEndpointToUniverse,
    &ManagementEndpoint::SetEndpointToUniverse},
  // PID_RDM_TRAFFIC_ENABLE
  { ola::rdm::PID_ENDPOINT_MODE,
    &ManagementEndpoint::GetEndpointMode,
    &ManagementEndpoint::SetEndpointMode},
  { ola::rdm::PID_ENDPOINT_LABEL,
    &ManagementEndpoint::GetEndpointLabel,
    &ManagementEndpoint::SetEndpointLabel},
  // PID_DISCOVERY_STATE
  // PID_BACKGROUND_DISCOVERY
  // PID_ENDPOINT_TIMING
  // PID_ENDPOINT_TIMING_DESCRIPTION
  { ola::rdm::PID_ENDPOINT_RESPONDER_LIST_CHANGE,
    &ManagementEndpoint::GetEndpointResponderListChange,
    NULL},
  { ola::rdm::PID_ENDPOINT_RESPONDERS,
    &ManagementEndpoint::GetEndpointResponders,
    NULL},
  // PID_BINDING_AND_CONTROL_FIELDS
  { ola::rdm::PID_TCP_COMMS_STATUS,
    &ManagementEndpoint::GetTCPCommsStatus,
    &ManagementEndpoint::SetTCPCommsStatus},
  // PID_BACKGROUND_QUEUED_STATUS_POLICY
  // PID_BACKGROUND_QUEUED_STATUS_POLICY_DESCRIPTION
  // PID_BACKGROUND_STATUS_TYPE
  // PID_QUEUED_STATUS_ENDPOINT_COLLECTION
  // PID_QUEUED_STATUS_UID_COLLECTION
};

/**
 * Create a new ManagementEndpoint. Ownership of the arguments is not taken.
 * The endpoint needs to out-live the controller since the controller may be
 * passed callbacks that reference this endpoint.
 */
ManagementEndpoint::ManagementEndpoint(
    DiscoverableRDMControllerInterface *controller,
    const EndpointProperties &properties,
    const ola::rdm::UID &uid,
    const class EndpointManager *endpoint_manager,
    class TCPConnectionStats *tcp_stats)
    : E133Endpoint(controller, properties),
      m_uid(uid),
      m_endpoint_manager(endpoint_manager),
      m_tcp_stats(tcp_stats),
      m_controller(controller) {
}


/**
 * Handle a RDM request by either passing it through or handling it internally.
 * @param request the RDMRequest object
 * @param on_complete the callback to run when we've handled this request.
 */
void ManagementEndpoint::SendRDMRequest(RDMRequest *request,
                                        RDMCallback *on_complete) {
  const UID dst_uid = request->DestinationUID();
  if (dst_uid.IsBroadcast() && m_controller) {
    // This request needs to go to both the E1.33 responder and the other
    // responders.
    // TODO(simon): We need to use a broadcast tracker here like in
    // Universe.cpp
    /*
    */
  } else if (request->DestinationUID().DirectedToUID(m_uid)) {
    RDMOps::Instance()->HandleRDMRequest(
        this, m_uid, ola::rdm::ROOT_RDM_DEVICE, request, on_complete);
  } else if (m_controller) {
    // This request just goes to the other responders.
    m_controller->SendRDMRequest(request, on_complete);
  } else {
    ola::rdm::RunRDMCallback(on_complete, ola::rdm::RDM_UNKNOWN_UID);
    delete request;
  }
}


/**
 * Run full discovery.
 */
void ManagementEndpoint::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  if (m_controller) {
    m_controller->RunFullDiscovery(
        NewSingleCallback(this, &ManagementEndpoint::DiscoveryComplete,
                          callback));
  } else {
    UIDSet uids;
    uids.AddUID(m_uid);
    callback->Run(uids);
  }
}


/**
 * Run incremental discovery.
 */
void ManagementEndpoint::RunIncrementalDiscovery(
    RDMDiscoveryCallback *callback) {
  if (m_controller) {
    m_controller->RunIncrementalDiscovery(
        NewSingleCallback(this, &ManagementEndpoint::DiscoveryComplete,
                          callback));
  } else {
    UIDSet uids;
    uids.AddUID(m_uid);
    callback->Run(uids);
  }
}


/**
 * Handle PID_ENDPOINT_LIST.
 */
RDMResponse *ManagementEndpoint::GetEndpointList(const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  struct EndpointListParamData {
    uint32_t list_change;
    uint16_t endpoint_id[0];
  };

  vector<uint16_t> endpoints;
  m_endpoint_manager->EndpointIDs(&endpoints);
  unsigned int param_data_size = 4 + 2 * endpoints.size();
  uint8_t *raw_data = new uint8_t[param_data_size];
  EndpointListParamData *param_data = reinterpret_cast<EndpointListParamData*>(
      raw_data);

  param_data->list_change = HostToNetwork(
      m_endpoint_manager->list_change_number());
  for (unsigned int i = 0; i < endpoints.size(); i++) {
    param_data->endpoint_id[i] = HostToNetwork(endpoints[i]);
  }

  RDMResponse *response = GetResponseFromData(request, raw_data,
                                              param_data_size);
  delete[] raw_data;
  return response;
}


/**
 * Handle PID_ENDPOINT_LIST_CHANGE.
 */
RDMResponse *ManagementEndpoint::GetEndpointListChange(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint32_t change = HostToNetwork(m_endpoint_manager->list_change_number());
  return GetResponseFromData(
      request, reinterpret_cast<uint8_t*>(&change), sizeof(change));
}


/**
 * Handle PID_IDENTIFY_ENDPOINT
 */
RDMResponse *ManagementEndpoint::GetIdentifyEndpoint(
    const RDMRequest *request) {
  uint16_t endpoint_id;
  if (!ResponderHelper::ExtractUInt16(request, &endpoint_id)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  E133Endpoint *endpoint = m_endpoint_manager->GetEndpoint(endpoint_id);

  // endpoint not found
  if (!endpoint) {
    return NackWithReason(request, ola::rdm::NR_ENDPOINT_NUMBER_INVALID);
  }

  PACK(
  struct IdentifyEndpointParamData {
    uint16_t endpoint_number;
    uint8_t identify_mode;
  });
  IdentifyEndpointParamData identify_endpoint_message = {
    HostToNetwork(endpoint_id), endpoint->identify_mode()
  };

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&identify_endpoint_message),
      sizeof(identify_endpoint_message));
}

RDMResponse *ManagementEndpoint::SetIdentifyEndpoint(
    const RDMRequest *request) {
  PACK(
  struct IdentifyEndpointParamData {
    uint16_t endpoint_number;
    uint8_t identify_mode;
  });
  IdentifyEndpointParamData identify_endpoint_message;

  if (request->ParamDataSize() != sizeof(identify_endpoint_message)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&identify_endpoint_message),
         request->ParamData(),
         sizeof(identify_endpoint_message));

  E133Endpoint *endpoint = m_endpoint_manager->GetEndpoint(
    ola::network::NetworkToHost(identify_endpoint_message.endpoint_number));
  // endpoint not found
  if (!endpoint) {
    return NackWithReason(request, ola::rdm::NR_ENDPOINT_NUMBER_INVALID);
  }

  endpoint->set_identify_mode(identify_endpoint_message.identify_mode);
  return GetResponseFromData(request, NULL, 0);
}


/**
 * Handle PID_ENDPOINT_TO_UNIVERSE
 */
RDMResponse *ManagementEndpoint::GetEndpointToUniverse(
    const RDMRequest *request) {
  // TODO(simon): add me
  return NackWithReason(request, NR_UNKNOWN_PID);
}

RDMResponse *ManagementEndpoint::SetEndpointToUniverse(
    const RDMRequest *request) {
  // TODO(simon): add me
  return NackWithReason(request, NR_UNKNOWN_PID);
}

/**
 * Handle PID_ENDPOINT_MODE
 */
RDMResponse *ManagementEndpoint::GetEndpointMode(const RDMRequest *request) {
  // TODO(simon): add me
  return NackWithReason(request, NR_UNKNOWN_PID);
}

RDMResponse *ManagementEndpoint::SetEndpointMode(const RDMRequest *request) {
  // TODO(simon): add me
  return NackWithReason(request, NR_UNKNOWN_PID);
}

/**
 * Handle PID_ENDPOINT_LABEL
 */
RDMResponse *ManagementEndpoint::GetEndpointLabel(const RDMRequest *request) {
  // TODO(simon): add me
  return NackWithReason(request, NR_UNKNOWN_PID);
}

RDMResponse *ManagementEndpoint::SetEndpointLabel(const RDMRequest *request) {
  // TODO(simon): add me
  return NackWithReason(request, NR_UNKNOWN_PID);
}

/**
 * Handle PID_ENDPOINT_RESPONDER_LIST_CHANGE
 */
RDMResponse *ManagementEndpoint::GetEndpointResponderListChange(
    const RDMRequest *request) {
  uint16_t endpoint_id;
  if (!ResponderHelper::ExtractUInt16(request, &endpoint_id)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  E133Endpoint *endpoint = m_endpoint_manager->GetEndpoint(endpoint_id);

  // endpoint not found
  if (!endpoint) {
    return NackWithReason(request, ola::rdm::NR_ENDPOINT_NUMBER_INVALID);
  }

  uint32_t list_change_id = HostToNetwork(endpoint->responder_list_change());
  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&list_change_id),
      sizeof(list_change_id));
}

/**
 * Handle PID_ENDPOINT_RESPONDERS
 */
RDMResponse *ManagementEndpoint::GetEndpointResponders(
    const RDMRequest *request) {
  uint16_t endpoint_id;
  if (!ResponderHelper::ExtractUInt16(request, &endpoint_id)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  E133Endpoint *endpoint = NULL;
  if (endpoint_id)
    endpoint = m_endpoint_manager->GetEndpoint(endpoint_id);
  else
    endpoint = this;

  OLA_INFO << "Endpoint ID: " << endpoint_id << ", endpoint " << endpoint;
  // endpoint not found
  if (!endpoint) {
    return NackWithReason(request, ola::rdm::NR_ENDPOINT_NUMBER_INVALID);
  }

  UIDSet uids;
  uint32_t list_change_id = HostToNetwork(endpoint->responder_list_change());
  endpoint->EndpointResponders(&uids);

  struct ResponderListParamData {
    uint16_t endpoint;
    uint32_t list_change;
    uint8_t data[0];
  };

  // TODO(simon): fix this - we can overflow an RDM packet if there are too
  // many responders!
  unsigned int param_data_size = 2 + 4 + uids.Size() * UID::UID_SIZE;
  uint8_t *raw_data = new uint8_t[param_data_size];
  ResponderListParamData *param_data =
      reinterpret_cast<ResponderListParamData*>(raw_data);

  // TODO(simon): fix this to track changes.
  param_data->endpoint = HostToNetwork(endpoint_id);
  param_data->list_change = list_change_id;
  uint8_t *ptr = raw_data + 6;
  unsigned int offset = 0;
  UIDSet::Iterator iter = uids.Begin();
  for (; iter != uids.End(); ++iter) {
    OLA_INFO << "  " << *iter;
    iter->Pack(ptr + offset, param_data_size - offset - 4);
    offset += UID::UID_SIZE;
  }

  RDMResponse *response = GetResponseFromData(request, raw_data,
                                              param_data_size);
  delete[] raw_data;
  return response;
}

/**
 * Handle PID_TCP_COMMS_STATUS
 */
RDMResponse *ManagementEndpoint::GetTCPCommsStatus(const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  PACK(
  struct tcp_stats_message_s {
    uint32_t ip_address;
    uint16_t unhealthy_events;
    uint16_t connection_events;
  });
  struct tcp_stats_message_s tcp_stats_message;

  tcp_stats_message.ip_address = m_tcp_stats->ip_address.AsInt();
  tcp_stats_message.unhealthy_events =
    HostToNetwork(m_tcp_stats->unhealthy_events);
  tcp_stats_message.connection_events =
    HostToNetwork(m_tcp_stats->connection_events);

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&tcp_stats_message),
      sizeof(tcp_stats_message));
}

RDMResponse *ManagementEndpoint::SetTCPCommsStatus(const RDMRequest *request) {
  m_tcp_stats->ResetCounters();
  return GetResponseFromData(request, NULL, 0);
}

/**
 * Add our UID to the set and run the Discovery Callback.
 */
void ManagementEndpoint::DiscoveryComplete(RDMDiscoveryCallback *callback,
                                           const UIDSet &uids) {
  UIDSet all_uids(uids);
  all_uids.AddUID(m_uid);
  callback->Run(all_uids);
}
