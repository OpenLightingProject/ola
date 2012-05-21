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
 * DummyResponder_h
 * The dummy responder is a simple software RDM responder. It's useful for
 * testing RDM controllers.
 * Copyright (C) 2009 Simon Newton
 */

#ifndef PLUGINS_DUMMY_DUMMYRESPONDER_H_
#define PLUGINS_DUMMY_DUMMYRESPONDER_H_

#include <vector>
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "plugins/dummy/DummyRDMDevice.h"

namespace ola {
namespace plugin {
namespace dummy {

class DummyResponder: public ola::rdm::RDMControllerInterface {
  public:
    DummyResponder(const ola::rdm::UID &uid, int number_of_devices);
    virtual ~DummyResponder();

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

    uint16_t RootDeviceFootprint() const {
      return m_root_devices[0]->Footprint();
    }

    const ola::rdm::UID &UID() const { return m_uid; }

  private:
    ola::rdm::UID m_uid;
    std::vector<DummyRDMDevice*> m_root_devices;
};
}  // dummy
}  // plugin
}  // ola
#endif  // PLUGINS_DUMMY_DUMMYRESPONDER_H_
