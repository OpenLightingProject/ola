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
 * DummyRDMDevice_h
 * Copyright (C) 2009 Simon Newton
 */

#ifndef PLUGINS_DUMMY_DUMMYRDMDEVICE_H_
#define PLUGINS_DUMMY_DUMMYRDMDEVICE_H_

#include <string>
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"

namespace ola {
namespace plugin {
namespace dummy {

class DummyRDMDevice: public ola::rdm::RDMControllerInterface {
  public:
    explicit DummyRDMDevice(uint16_t sub_device_number):
      m_start_address(1),
      m_personality(0),
      m_identify_mode(0),
      m_lamp_strikes(0),
      sub_device_num(sub_device_number) {}

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

    uint16_t DeviceNumber() const { return sub_device_number; }
    uint16_t StartAddress() const { return m_start_address; }
    uint16_t Footprint() const {
      return PERSONALITIES[m_personality].footprint;
    }

  private:
    uint16_t m_start_address;
    uint8_t m_personality;
    uint8_t m_identify_mode;
    uint32_t m_lamp_strikes;
    uint16_t sub_device_number;

    void HandleUnknownPacket(const ola::rdm::RDMRequest *request,
                             ola::rdm::RDMCallback *callback);
    void HandleSupportedParams(const ola::rdm::RDMRequest *request,
                               ola::rdm::RDMCallback *callback);
    void HandleDeviceInfo(const ola::rdm::RDMRequest *request,
                          ola::rdm::RDMCallback *callback);
    void HandleFactoryDefaults(const ola::rdm::RDMRequest *request,
                               ola::rdm::RDMCallback *callback);
    void HandleProductDetailList(const ola::rdm::RDMRequest *request,
                                 ola::rdm::RDMCallback *callback);
    void HandleStringResponse(const ola::rdm::RDMRequest *request,
                              ola::rdm::RDMCallback *callback,
                              const std::string &value);
    void HandlePersonality(const ola::rdm::RDMRequest *request,
                           ola::rdm::RDMCallback *callback);
    void HandlePersonalityDescription(const ola::rdm::RDMRequest *request,
                                      ola::rdm::RDMCallback *callback);
    void HandleDmxStartAddress(const ola::rdm::RDMRequest *request,
                               ola::rdm::RDMCallback *callback);
    void HandleLampStrikes(const ola::rdm::RDMRequest *request,
                           ola::rdm::RDMCallback *callback);
    void HandleIdentifyDevice(const ola::rdm::RDMRequest *request,
                              ola::rdm::RDMCallback *callback);
    void HandleRealTimeClock(const ola::rdm::RDMRequest *request,
                             ola::rdm::RDMCallback *callback);
    bool CheckForBroadcastSubdeviceOrData(const ola::rdm::RDMRequest *request,
                                          ola::rdm::RDMCallback *callback);
    void RunRDMCallback(ola::rdm::RDMCallback *callback,
                        ola::rdm::RDMResponse *response);
    typedef struct {
      uint16_t footprint;
      const char *description;
    } personality_info;

    static const personality_info PERSONALITIES[];
    static const unsigned int PERSONALITY_COUNT;
};
}  // dummy
}  // plugin
}  // ola
#endif  // PLUGINS_DUMMY_DUMMYRDMDEVICE_H_
