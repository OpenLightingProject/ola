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
 * SubDeviceDispatcher.cpp
 * Handles dispatching RDM requests to the correct sub device.
 * Copyright (C) 2013 Simon Newton
 */

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "ola/stl/STLUtils.h"
#include "ola/Logging.h"
#include "plugins/dummy/SubDeviceDispatcher.h"

namespace ola {
namespace plugin {
namespace dummy {

using ola::rdm::RDMControllerInterface;
using ola::rdm::RDMRequest;
using ola::rdm::RDMCallback;
using std::string;
using std::vector;


/**
 * Add or remove a sub device. Ownership of the device is not transferred.
 */
void SubDeviceDispatcher::AddSubDevice(uint16_t sub_device_number,
                                       RDMControllerInterface *device) {
  STLReplace(&m_subdevices, sub_device_number, device);
}

/*
 * Handle an RDM Request
 */
void SubDeviceDispatcher::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                         RDMCallback *callback) {
  if (request->SubDevice() == ola::rdm::ALL_RDM_SUBDEVICES) {
    FanOutToSubDevices(request, callback);
  } else {
    RDMControllerInterface *device = STLFindOrNull(
        m_subdevices, request->SubDevice());
    if (device) {
      device->SendRDMRequest(request, callback);
    } else {
      NackIfNotBroadcast(request, callback,
                         ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
    }
  }
}


/**
 * Handle commands sent to the SUB_DEVICE_ALL_CALL target.
 */
void SubDeviceDispatcher::FanOutToSubDevices(
    const ola::rdm::RDMRequest *request,
    RDMCallback *callback) {
  // GETs to the all subdevices don't make any sense.
  // Section 9.2.2
  if (request->CommandClass() == ola::rdm::RDMCommand::GET_COMMAND) {
    NackIfNotBroadcast(request, callback,
                       ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
    return;
  }

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
    const ola::rdm::RDMRequest *request_ptr,
    RDMCallback *callback,
    ola::rdm::rdm_nack_reason nack_reason) {
  std::auto_ptr<const RDMRequest> request(request_ptr);
  vector<string> packets;
  if (request->DestinationUID().IsBroadcast()) {
    callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
  } else {
    ola::rdm::RDMResponse *response = ola::rdm::NackWithReason(
        request.get(), nack_reason);
    callback->Run(ola::rdm::RDM_COMPLETED_OK, response, packets);
  }
}

/**
 * Called when a subdevice returns during a ALL_RDM_SUBDEVICES call.
 */
void SubDeviceDispatcher::HandleSubDeviceResponse(
    FanOutTracker *tracker,
    uint16_t sub_device_id,
    ola::rdm::rdm_response_code code,
    const ola::rdm::RDMResponse *response_ptr,
    const std::vector<std::string> &packets) {
  std::auto_ptr<const ola::rdm::RDMResponse> response(response_ptr);

  if (tracker->IncrementAndCheckIfComplete()) {
    // now it's not really clear what we're supposed to return here.
    // We do the least crazy thing, which is to return the root device response.
    tracker->RunCallback();
    delete tracker;
  } else if (sub_device_id == ola::rdm::ROOT_RDM_DEVICE) {
    tracker->SetRootResponse(code, response.release());
  }
  (void) packets;
}

SubDeviceDispatcher::FanOutTracker::FanOutTracker(
    uint16_t number_of_subdevices,
    ola::rdm::RDMCallback *callback)
    : m_number_of_subdevices(number_of_subdevices),
      m_responses_so_far(0),
      m_callback(callback),
      m_response(NULL) {
}

void SubDeviceDispatcher::FanOutTracker::SetRootResponse(
    ola::rdm::rdm_response_code code,
    const ola::rdm::RDMResponse *response) {
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
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
