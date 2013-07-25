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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
  }
  OLA_WARN << "SubDeviceDispatcher does not accept Root Devices";
}

/*
 * Handle an RDM Request
 */
void SubDeviceDispatcher::SendRDMRequest(const RDMRequest *request,
                                         RDMCallback *callback) {
  if (request->SubDevice() == ALL_RDM_SUBDEVICES) {
    FanOutToSubDevices(request, callback);
  } else {
    RDMControllerInterface *device = STLFindOrNull(
        m_subdevices, request->SubDevice());
    if (device) {
      device->SendRDMRequest(request, callback);
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
  SubDeviceMap::iterator iter = m_subdevices.begin();
  FanOutTracker *tracker = new FanOutTracker(m_subdevices.size(), callback);

  for (; iter != m_subdevices.end(); ++iter) {
    iter->second->SendRDMRequest(
        request->Duplicate(),
        NewSingleCallback(this,
                          &SubDeviceDispatcher::HandleSubDeviceResponse,
                          tracker,
                          iter->first));
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
  vector<string> packets;
  if (request->DestinationUID().IsBroadcast()) {
    callback->Run(RDM_WAS_BROADCAST, NULL, packets);
  } else {
    RDMResponse *response = NackWithReason(
        request.get(), nack_reason);
    callback->Run(RDM_COMPLETED_OK, response, packets);
  }
}

/**
 * Called when a subdevice returns during a ALL_RDM_SUBDEVICES call.
 */
void SubDeviceDispatcher::HandleSubDeviceResponse(
    FanOutTracker *tracker,
    uint16_t sub_device_id,
    rdm_response_code code,
    const RDMResponse *response_ptr,
    const std::vector<std::string> &packets) {
  std::auto_ptr<const RDMResponse> response(response_ptr);

  if (tracker->NumResponses() == 0) {
    tracker->SetResponse(code, response.release());
  }

  if (tracker->IncrementAndCheckIfComplete()) {
    // now it's not really clear what we're supposed to return here.
    // We do the least crazy thing, which is to return the root device response.
    tracker->RunCallback();
    delete tracker;
  }
  (void) sub_device_id;
  (void) packets;
}

SubDeviceDispatcher::FanOutTracker::FanOutTracker(
    uint16_t number_of_subdevices,
    RDMCallback *callback)
    : m_number_of_subdevices(number_of_subdevices),
      m_responses_so_far(0),
      m_callback(callback),
      m_response(NULL) {
}

void SubDeviceDispatcher::FanOutTracker::SetResponse(
    rdm_response_code code,
    const RDMResponse *response) {
  m_response_code = code;
  m_response = response;
}

void SubDeviceDispatcher::FanOutTracker::RunCallback() {
  vector<string> packets;
  if (m_callback) {
    m_callback->Run(m_response_code, m_response, packets);
  }
  m_callback = NULL;
}
}  // namespace rdm
}  // namespace ola
