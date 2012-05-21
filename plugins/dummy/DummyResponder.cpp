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
 * DummyResponder.cpp
 * The Dummy Responder for ola
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <iostream>
#include "plugins/dummy/DummyResponder.h"

namespace ola {
namespace plugin {
namespace dummy {

DummyResponder::DummyResponder(const ola::rdm::UID &uid, int number_of_devices) {
  m_uid = uid;
  for (int i = 0; i < number_of_devices; i++) {
    m_root_devices[i] = new DummyRDMDevice(m_uid, ola::rdm::ROOT_RDM_DEVICE + i);
  }
}

/*
 * Handle an RDM Request
 */
void DummyResponder::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                    ola::rdm::RDMCallback *callback) {
  m_root_device.SendRDMRequest(request, callback);
}

DummyResponder::~DummyResponder() {
  for (unsigned i = 0; i < m_root_devices.size(); i++) {
    delete m_root_devices[i];
  }
}
}  // dummy
}  // plugin
}  // ola
