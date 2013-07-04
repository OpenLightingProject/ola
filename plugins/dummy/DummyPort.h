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
 * DummyPort.h
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

using ola::rdm::UID;

namespace ola {
namespace plugin {
namespace dummy {

class DummyPort: public BasicOutputPort {
  public:
    struct Options {
      public:
        Options()
            : number_of_dimmers(1),
              dimmer_sub_device_count(4),
              number_of_moving_lights(1),
              number_of_dummy_responders(1),
              number_of_ack_timer_responders(0) {
        }

        uint8_t number_of_dimmers;
        uint16_t dimmer_sub_device_count;
        uint8_t number_of_moving_lights;
        uint8_t number_of_dummy_responders;
        uint8_t number_of_ack_timer_responders;
    };

    DummyPort(class DummyDevice *parent,
              const Options &options,
              unsigned int id);
    virtual ~DummyPort();
    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    string Description() const { return "Dummy Port"; }
    void RunFullDiscovery(RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(RDMDiscoveryCallback *callback);
    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

  private:
    typedef struct {
      unsigned int expected_count;
      unsigned int current_count;
      bool failed;
      ola::rdm::RDMCallback *callback;
    } broadcast_request_tracker;

    typedef map<UID, ola::rdm::RDMControllerInterface*> ResponderMap;

    DmxBuffer m_buffer;
    ResponderMap m_responders;

    void RunDiscovery(RDMDiscoveryCallback *callback);
    void HandleBroadcastAck(broadcast_request_tracker *tracker,
                            ola::rdm::rdm_response_code code,
                            const ola::rdm::RDMResponse *response,
                            const std::vector<std::string> &packets);

    // See http://www.opendmx.net/index.php/Open_Lighting_Allocations
    // Do not change.
    static const unsigned int kStartAddress = 0xffffff00;
};
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_DUMMY_DUMMYPORT_H_
