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
#include "ola/DmxBuffer.h"
#include "ola/rdm/RDMEnums.h"
#include "olad/Port.h"
#include "plugins/dummy/DummyDevice.h"

namespace ola {
namespace plugin {
namespace dummy {

class DummyPort: public BasicOutputPort {
  public:
    DummyPort(DummyDevice *parent, unsigned int id):
      BasicOutputPort(parent, id, true),
      m_start_address(1) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    string Description() const { return "Dummy Port"; }
    void RunRDMDiscovery();
    bool HandleRDMRequest(const ola::rdm::RDMRequest *request);

  private:
    bool HandleUnknownPacket(const ola::rdm::RDMRequest *request);
    bool HandleSupportedParams(const ola::rdm::RDMRequest *request);
    bool HandleDeviceInfo(const ola::rdm::RDMRequest *request);
    bool HandleProductDetailList(const ola::rdm::RDMRequest *request);
    bool HandleStringResponse(const ola::rdm::RDMRequest *request,
                              const string &value);
    bool HandleDmxStartAddress(const ola::rdm::RDMRequest *request);

    uint16_t m_start_address;
    DmxBuffer m_buffer;

    static const uint16_t DUMMY_DMX_FOOTPRINT = 10;
};
}  // dummy
}  // plugin
}  // ola
#endif  // PLUGINS_DUMMY_DUMMYPORT_H_
