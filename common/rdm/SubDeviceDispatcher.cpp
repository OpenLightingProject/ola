/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * SubDeviceDispatcher.cpp
 * Handles dispatching RDM requests to the correct sub device.
 * Copyright (C) 2013 Simon Newton
 */

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/rdm/SubDeviceDispatcher.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace rdm {

using std::string;
using std::vector;

/**
 * Add or remove a sub device. Ownership of the device is not transferred.
 */
void SubDeviceDispatcher::AddSubDevice(uint16_t sub_device_number,
                                       RDMControllerInterface *device) {
  if (sub_device_number != ROOT_RDM_DEVICE) {
    STLReplace(&m_subdevices, sub_device_number, device);
  } else {
    OLA_WARN << "SubDeviceDispatcher does not accept Root Devices";
  }
}

/*
 * Handle an RDM Request
 */
void SubDeviceDispatcher::SendRDMRequest(RDMRequest *request,
                                         RDMCallback *callback) {
  if (request->SubDevice() == ALL_RDM_SUBDEVICES) {
    FanOutToSubDevices(request, callback);
  } else {
    RDMControllerInterface *sub_device = STLFindOrNull(
        m_subdevices, request->SubDevice());
    if (sub_device) {
      sub_device->SendRDMRequest(request, callback);
    } else {
      NackIfNotBroadcast(request, callback, NR_SUB_DEVICE_OUT_OF_RANGE);
    }
  }
}


/**
 * Handle commands sent to the SUB_DEVICE_ALL_CALL target.
 */
void SubDeviceDispatcher::FanOutToSubDevices(
    const RDMRequest *request,
    RDMCallback *callback) {
  // GETs to the all subdevices don't make any sense.
  // Section 9.2.2
  if (request->CommandClass() == RDMCommand::GET_COMMAND) {
    NackIfNotBroadcast(request, callback, NR_SUB_DEVICE_OUT_OF_RANGE);
    return;
  }

  // Fan out to all sub devices but don't include the root device
  if (m_subdevices.empty()) {
    RunRDMCallback(callback, RDM_WAS_BROADCAST);
  } else {
    SubDeviceMap::iterator iter = m_subdevices.begin();
    FanOutTracker *tracker = new FanOutTracker(m_subdevices.size(), callback);

    for (; iter != m_subdevices.end(); ++iter) {
      iter->second->SendRDMRequest(
          request->Duplicate(),
          NewSingleCallback(this,
                            &SubDeviceDispatcher::HandleSubDeviceResponse,
                            tracker));
    }
  }
}


/**
 * Respond with a NACK, or RDM_WAS_BROADCAST.
 * Takes ownership of the request object.
 */
void SubDeviceDispatcher::NackIfNotBroadcast(
    const RDMRequest *request_ptr,
    RDMCallback *callback,
    rdm_nack_reason nack_reason) {
  std::auto_ptr<const RDMRequest> request(request_ptr);
  if (request->DestinationUID().IsBroadcast()) {
    RunRDMCallback(callback, RDM_WAS_BROADCAST);
  } else {
    RDMReply reply(RDM_COMPLETED_OK,
                   NackWithReason(request.get(), nack_reason));
    callback->Run(&reply);
  }
}

/**
 * Called when a subdevice returns during a ALL_RDM_SUBDEVICES call.
 */
void SubDeviceDispatcher::HandleSubDeviceResponse(FanOutTracker *tracker,
                                                  RDMReply *reply) {
  if (tracker->NumResponses() == 0) {
    tracker->SetResponse(reply->StatusCode(), reply->Response()->Duplicate());
  }

  if (tracker->IncrementAndCheckIfComplete()) {
    // now it's not really clear what we're supposed to return here.
    // We do the least crazy thing, which is to return the root device response.
    tracker->RunCallback();
    delete tracker;
  }
}

SubDeviceDispatcher::FanOutTracker::FanOutTracker(
    uint16_t number_of_subdevices,
    RDMCallback *callback)
    : m_number_of_subdevices(number_of_subdevices),
      m_responses_so_far(0),
      m_callback(callback),
      m_status_code(RDM_COMPLETED_OK),
      m_response(NULL) {
}

void SubDeviceDispatcher::FanOutTracker::SetResponse(
    RDMStatusCode code,
    RDMResponse *response) {
  m_status_code = code;
  m_response = response;
}

void SubDeviceDispatcher::FanOutTracker::RunCallback() {
  if (m_callback) {
    RDMReply reply(m_status_code, m_response);
    m_callback->Run(&reply);
  }
  m_callback = NULL;
}
}  // namespace rdm
}  // namespace ola
