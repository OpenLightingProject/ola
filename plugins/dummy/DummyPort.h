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
#include <map>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/DmxBuffer.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "olad/Port.h"
#include "plugins/dummy/DummyDevice.h"
#include "plugins/dummy/DummyResponder.h"

using ola::rdm::UID;

namespace ola {
namespace plugin {
namespace dummy {

class DummyPort: public BasicOutputPort {
  public:
    DummyPort(DummyDevice *parent,
              unsigned int id,
              uint16_t device_count,
              uint16_t subdevice_count);
    virtual ~DummyPort();
    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    string Description() const { return "Dummy Port"; }
    void RunFullDiscovery(RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(RDMDiscoveryCallback *callback);
    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);
    static const unsigned int kStartAddress = 0xffffff00;

  private:
    typedef struct {
      unsigned int expected_count;
      unsigned int current_count;
      bool failed;
      ola::rdm::RDMCallback *callback;
    } broadcast_request_tracker;

    typedef map<UID, DummyResponder *> ResponderMap;
    void RunDiscovery(RDMDiscoveryCallback *callback);
    void HandleBroadcastAck(broadcast_request_tracker *tracker,
                            ola::rdm::rdm_response_code code,
                            const ola::rdm::RDMResponse *response,
                            const std::vector<std::string> &packets);

    DmxBuffer m_buffer;
    ResponderMap m_responders;
};
}  // dummy
}  // plugin
}  // ola
#endif  // PLUGINS_DUMMY_DUMMYPORT_H_
