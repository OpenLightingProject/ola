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
 * DimmerResponder.cpp
 * A fake dimmer responder for OLA.
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/stl/STLUtils.h>
#include <algorithm>
#include <map>
#include "ola/rdm/DimmerResponder.h"
#include "ola/rdm/DimmerRootDevice.h"
#include "ola/rdm/DimmerSubDevice.h"

namespace ola {
namespace rdm {

/**
 * Create a new dummy dimmer responder.
 * @param uid the UID of the responder
 * @param number_of_subdevices the number of sub devices for this responder.
 * Valid range is 0 to 512.
 */
DimmerResponder::DimmerResponder(const UID &uid,
                                 uint16_t number_of_subdevices) {
  uint16_t sub_devices = std::min(MAX_SUBDEVICE_NUMBER,
                                  number_of_subdevices);
  for (uint16_t i = 1; i <= sub_devices; i++) {
    DimmerSubDevice *sub_device = new DimmerSubDevice(
        uid, i, sub_devices);
    STLInsertIfNotPresent(&m_sub_devices, i, sub_device);
    m_dispatcher.AddSubDevice(i, sub_device);
  }
  m_root_device.reset(new DimmerRootDevice(uid, m_sub_devices));
}

/**
 * Cleanup this responder
 */
DimmerResponder::~DimmerResponder() {
  STLDeleteValues(&m_sub_devices);
}

/*
 * Handle an RDM Request. This just uses the SubDeviceDispatcher to call the
 * correct sub device.
 */
void DimmerResponder::SendRDMRequest(const RDMRequest *request,
                                     RDMCallback *callback) {
  if (request->SubDevice() == ROOT_RDM_DEVICE) {
    m_root_device->SendRDMRequest(request, callback);
  } else {
    m_dispatcher.SendRDMRequest(request, callback);
  }
}
}  // namespace rdm
}  // namespace ola
