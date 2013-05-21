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
 * ManagementEndpoint.cpp
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
#include "tools/e133/ManagementEndpoint.h"
#include "tools/e133/TCPConnectionStats.h"

using ola::network::HostToNetwork;
using ola::rdm::RDMCallback;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::auto_ptr;
using std::vector;


typedef std::vector<std::string> RDMPackets;

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
void ManagementEndpoint::SendRDMRequest(const RDMRequest *request,
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
    // This request just goes to the E1.33 responder,
    HandleManagementRequest(request, on_complete);
  } else if (m_controller) {
    // This request just goes to the other responders.
    m_controller->SendRDMRequest(request, on_complete);
  } else {
    RDMPackets packets;
    delete request;
    on_complete->Run(ola::rdm::RDM_UNKNOWN_UID, NULL, packets);
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
 * Handle a request dirrected at the Management UID.
 */
void ManagementEndpoint::HandleManagementRequest(const RDMRequest *request_ptr,
                                                 RDMCallback *on_complete) {
  auto_ptr<const RDMRequest> request(request_ptr);

  const UID dst_uid = request->DestinationUID();
  bool for_us = dst_uid.DirectedToUID(m_uid);

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
    case ola::rdm::PID_ENDPOINT_LIST_CHANGE:
      HandleEndpointListChange(request.get(), on_complete);
      break;
    case ola::rdm::PID_ENDPOINT_IDENTIFY:
      HandleEndpointIdentify(request.get(), on_complete);
      break;
    case ola::rdm::PID_ENDPOINT_TO_UNIVERSE:
      HandleEndpointToUniverse(request.get(), on_complete);
      break;
    case ola::rdm::PID_ENDPOINT_MODE:
      HandleEndpointMode(request.get(), on_complete);
      break;
    case ola::rdm::PID_ENDPOINT_LABEL:
      HandleEndpointLabel(request.get(), on_complete);
      break;
    case ola::rdm::PID_ENDPOINT_DEVICE_LIST_CHANGE:
      HandleEndpointDeviceListChange(request.get(), on_complete);
      break;
    case ola::rdm::PID_ENDPOINT_DEVICES:
      HandleEndpointDevices(request.get(), on_complete);
      break;
    // TODO(simon): add PID_BINDING_AND_CONTROL_FIELDS.
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
void ManagementEndpoint::HandleSupportedParams(const RDMRequest *request,
                                               RDMCallback *on_complete) {
  if (!SanityCheckGet(request, on_complete, 0))
    return;

  uint16_t supported_params[] = {
    ola::rdm::PID_ENDPOINT_LIST,
    ola::rdm::PID_ENDPOINT_LIST_CHANGE,
    ola::rdm::PID_ENDPOINT_IDENTIFY,
    ola::rdm::PID_ENDPOINT_TO_UNIVERSE,
    ola::rdm::PID_ENDPOINT_MODE,
    ola::rdm::PID_ENDPOINT_LABEL,
    ola::rdm::PID_ENDPOINT_DEVICE_LIST_CHANGE,
    ola::rdm::PID_ENDPOINT_DEVICES,
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
void ManagementEndpoint::HandleEndpointList(const RDMRequest *request,
                                            RDMCallback *on_complete) {
  if (!SanityCheckGet(request, on_complete, 0))
    return;

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
  RunRDMCallback(on_complete, response);
}


/**
 * Handle PID_ENDPOINT_LIST_CHANGE.
 */
void ManagementEndpoint::HandleEndpointListChange(const RDMRequest *request,
                                                  RDMCallback *on_complete) {
  if (!SanityCheckGet(request, on_complete, 0))
    return;

  uint32_t change = HostToNetwork(m_endpoint_manager->list_change_number());
  RDMResponse *response = GetResponseFromData(
      request, reinterpret_cast<uint8_t*>(&change), sizeof(change));
  RunRDMCallback(on_complete, response);
}


/**
 * Handle PID_ENDPOINT_IDENTIFY
 */
void ManagementEndpoint::HandleEndpointIdentify(const RDMRequest *request,
                                                RDMCallback *on_complete) {
  struct IdentifyEndpointParamData {
    uint16_t endpoint_number;
    uint8_t identify_mode;
  } __attribute__((packed));
  IdentifyEndpointParamData endpoint_identify_message;

  RDMPackets packets;
  uint16_t endpoint_id;
  if (!SanityCheckGetOrSet(request,
                           on_complete,
                           sizeof(endpoint_id),
                           sizeof(endpoint_identify_message),
                           sizeof(endpoint_identify_message)))
    return;

  memcpy(reinterpret_cast<uint8_t*>(&endpoint_id),
         request->ParamData(), sizeof(endpoint_id));
  endpoint_id = HostToNetwork(endpoint_id);

  E133Endpoint *endpoint = m_endpoint_manager->GetEndpoint(endpoint_id);

  // endpoint not found
  if (!endpoint) {
    if (request->DestinationUID().IsBroadcast()) {
      on_complete->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
      return;
    } else {
      RDMResponse *response = NackWithReason(
          request, ola::rdm::NR_ENDPOINT_NUMBER_INVALID);
      RunRDMCallback(on_complete, response);
      return;
    }
  }

  if (request->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND) {
    // SET
    memcpy(reinterpret_cast<uint8_t*>(&endpoint_identify_message),
           request->ParamData(),
           sizeof(endpoint_identify_message));

    endpoint->set_identify_mode(endpoint_identify_message.identify_mode);

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
    endpoint_identify_message.identify_mode = endpoint->identify_mode();
    RDMResponse *response = GetResponseFromData(
        request,
        reinterpret_cast<uint8_t*>(&endpoint_identify_message),
        sizeof(endpoint_identify_message));

    RunRDMCallback(on_complete, response);
  }
}


/**
 * Handle PID_ENDPOINT_TO_UNIVERSE
 */
void ManagementEndpoint::HandleEndpointToUniverse(const RDMRequest *request,
                                                  RDMCallback *on_complete) {
  // TODO(simon): add me
  HandleUnknownPID(request, on_complete);
}

/**
 * Handle PID_ENDPOINT_MODE
 */
void ManagementEndpoint::HandleEndpointMode(const RDMRequest *request,
                                            RDMCallback *on_complete) {
  // TODO(simon): add me
  HandleUnknownPID(request, on_complete);
}

/**
 * Handle PID_ENDPOINT_LABEL
 */
void ManagementEndpoint::HandleEndpointLabel(const RDMRequest *request,
                                             RDMCallback *on_complete) {
  // TODO(simon): add me
  HandleUnknownPID(request, on_complete);
}

/**
 * Handle PID_ENDPOINT_DEVICE_LIST_CHANGE
 */
void ManagementEndpoint::HandleEndpointDeviceListChange(
    const RDMRequest *request,
    RDMCallback *on_complete) {
  // TODO(simon): add me
  HandleUnknownPID(request, on_complete);
}

/**
 * Handle PID_ENDPOINT_DEVICES
 */
void ManagementEndpoint::HandleEndpointDevices(const RDMRequest *request,
                                               RDMCallback *on_complete) {
  RDMPackets packets;
  uint16_t endpoint_id;
  if (!SanityCheckGet(request, on_complete, sizeof(endpoint_id)))
    return;

  memcpy(reinterpret_cast<uint8_t*>(&endpoint_id),
         request->ParamData(), sizeof(endpoint_id));
  endpoint_id = HostToNetwork(endpoint_id);

  E133Endpoint *endpoint = NULL;
  if (endpoint_id)
    endpoint = m_endpoint_manager->GetEndpoint(endpoint_id);
  else
    endpoint = this;

  OLA_INFO << "Endpoint ID: " << endpoint_id << ", endpoint " << endpoint;

  // endpoint not found
  if (!endpoint) {
    if (request->DestinationUID().IsBroadcast()) {
      on_complete->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
      return;
    } else {
      RDMResponse *response = NackWithReason(
          request, ola::rdm::NR_ENDPOINT_NUMBER_INVALID);
      RunRDMCallback(on_complete, response);
      return;
    }
  }

  endpoint->RunIncrementalDiscovery(
      NewSingleCallback(this, &ManagementEndpoint::EndpointDevicesComplete,
                        request->Duplicate(), on_complete, endpoint_id));
}

/**
 * Handle PID_TCP_COMMS_STATUS
 */
void ManagementEndpoint::HandleTCPCommsStatus(const RDMRequest *request,
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
    m_tcp_stats->ResetCounters();

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
void ManagementEndpoint::HandleUnknownPID(const RDMRequest *request,
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
bool ManagementEndpoint::SanityCheckGet(const RDMRequest *request,
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
bool ManagementEndpoint::SanityCheckGetOrSet(
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
 * Run the RDM callback with a response.
 */
void ManagementEndpoint::RunRDMCallback(RDMCallback *callback,
                                        RDMResponse *response) {
  RDMPackets packets;
  callback->Run(ola::rdm::RDM_COMPLETED_OK, response, packets);
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


/**
 * Called when a Endpoint discovery completes
 */
void ManagementEndpoint::EndpointDevicesComplete(
    RDMRequest *request,
    RDMCallback *on_complete,
    uint16_t endpoint,
    const UIDSet &uids) {
  OLA_INFO << endpoint << " Devices complete, " << uids.Size() << " uids";
  // TODO(simon): fix this hack.

  struct DeviceListParamData {
    uint16_t endpoint;
    uint32_t list_change;
    uint8_t data[0];
  };

  // TODO(simon): fix this - we can overflow an RDM packet if there are too
  // many devices!
  unsigned int param_data_size = 2 + 4 + uids.Size() * UID::UID_SIZE;
  uint8_t *raw_data = new uint8_t[param_data_size];
  DeviceListParamData *param_data = reinterpret_cast<DeviceListParamData*>(
      raw_data);

  // TODO(simon): fix this to track changes.
  param_data->endpoint = HostToNetwork(endpoint);
  param_data->list_change = HostToNetwork(0);
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
  RunRDMCallback(on_complete, response);
}
