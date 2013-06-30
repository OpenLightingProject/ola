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
 * DummyResponder_h
 * The dummy responder is a simple software RDM responder. It's useful for
 * testing RDM controllers.
 * Copyright (C) 2009 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_DUMMYRESPONDER_H_
#define INCLUDE_OLA_RDM_DUMMYRESPONDER_H_

#include <vector>
#include "ola/rdm/DummyRDMDevice.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace rdm {

class DummyResponder: public RDMControllerInterface {
  public:
    DummyResponder(const UID &uid, unsigned int number_of_subdevices = 0);
    virtual ~DummyResponder();

    void SendRDMRequest(const RDMRequest *request, RDMCallback *callback);

  private:
    UID m_uid;
    std::vector<DummyRDMDevice*> m_subdevices;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_DUMMYRESPONDER_H_
