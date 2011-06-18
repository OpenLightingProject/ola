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
 * DummyPort_h
 * The interface to the Dummy port
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef PLUGINS_DUMMY_DUMMYPORT_H_
#define PLUGINS_DUMMY_DUMMYPORT_H_

#include <string>
#include "ola/BaseTypes.h"
#include "ola/DmxBuffer.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "olad/Port.h"
#include "plugins/dummy/DummyDevice.h"
#include "plugins/dummy/DummyResponder.h"

namespace ola {
namespace plugin {
namespace dummy {

class DummyPort: public BasicOutputPort {
  public:
    DummyPort(DummyDevice *parent, unsigned int id):
      BasicOutputPort(parent, id, true),
      m_responder(ola::rdm::UID(OPEN_LIGHTING_ESTA_CODE, 0xffffff00)) {
    }

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    string Description() const { return "Dummy Port"; }
    void RunFullDiscovery();
    void RunIncrementalDiscovery();
    void HandleRDMRequest(const ola::rdm::RDMRequest *request,
                          ola::rdm::RDMCallback *callback);

  private:
    void RunDiscovery();

    DmxBuffer m_buffer;
    DummyResponder m_responder;
};
}  // dummy
}  // plugin
}  // ola
#endif  // PLUGINS_DUMMY_DUMMYPORT_H_
