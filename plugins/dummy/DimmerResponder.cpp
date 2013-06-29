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
 * DimmerResponder.cpp
 * The Dummy Responder for ola
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/stl/STLUtils.h>
#include <algorithm>
#include <map>
#include "plugins/dummy/DimmerResponder.h"

namespace ola {
namespace plugin {
namespace dummy {

/**
 * Create a new dummy dimmer responder.
 * @param uid the UID of the responder
 * @param number_of_subdevices the number of sub devices for this responder.
 * Valid range is 0 to 512.
 */
DimmerResponder::DimmerResponder(const ola::rdm::UID &uid,
                                 uint16_t number_of_subdevices)
    : m_uid(uid) {
  uint16_t sub_devices = std::min(ola::rdm::MAX_SUBDEVICE_NUMBER,
                                  number_of_subdevices);
  for (uint16_t i = 1; i <= sub_devices; i++) {
    DimmerSubDevice *sub_device = new DimmerSubDevice(uid, i);
    STLInsertIfNotPresent(&m_sub_devices, i, sub_device);
    m_dispatcher.AddSubDevice(i, sub_device);
  }
  m_root_device.reset(new DimmerRootDevice(uid, m_sub_devices));
  m_dispatcher.AddSubDevice(ola::rdm::ROOT_RDM_DEVICE, m_root_device.get());
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
void DimmerResponder::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                     ola::rdm::RDMCallback *callback) {
  m_dispatcher.SendRDMRequest(request, callback);
}
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
