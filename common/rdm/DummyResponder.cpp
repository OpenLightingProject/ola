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
 * DummyResponder.cpp
 * The Dummy Responder for ola
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include "ola/rdm/DummyResponder.h"

namespace ola {
namespace rdm {

/**
 * Create a new dummy responder
 * @param uid the UID of the responder
 * @param number_of_subdevices the number of sub devices for this responder.
 * Valid range is 0 to 254 for now.
 */
DummyResponder::DummyResponder(const UID &uid,
                               unsigned int number_of_subdevices)
    : m_uid(uid) {
  number_of_subdevices++;  // add one for the root device
  for (unsigned int i = 0; i < std::min(number_of_subdevices, 255u); i++) {
    m_subdevices.push_back(
        new DummyRDMDevice(m_uid, ROOT_RDM_DEVICE + i));
  }
}


/**
 * Cleanup this responder
 */
DummyResponder::~DummyResponder() {
  for (std::vector<DummyRDMDevice*>::iterator i = m_subdevices.begin();
       i != m_subdevices.end(); i++) {
    delete *i;
  }
}


/*
 * Handle an RDM Request
 */
void DummyResponder::SendRDMRequest(const RDMRequest *request,
                                    RDMCallback *callback) {
  m_subdevices[0]->SendRDMRequest(request, callback);
}
}  // namespace rdm
}  // namespace ola
