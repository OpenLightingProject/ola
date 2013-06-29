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
 * DummyRDMDevice_h
 * Copyright (C) 2009 Simon Newton
 */

#ifndef PLUGINS_DUMMY_DUMMYRDMDEVICE_H_
#define PLUGINS_DUMMY_DUMMYRDMDEVICE_H_

#include <string>
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderOps.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace plugin {
namespace dummy {

using ola::rdm::RDMResponse;

class DummyRDMDevice: public ola::rdm::RDMControllerInterface {
  public:
    DummyRDMDevice(const ola::rdm::UID &uid, uint16_t sub_device_number)
        : m_uid(uid),
          m_start_address(1),
          m_personality(1),
          m_identify_mode(0),
          m_lamp_strikes(0),
          m_sub_device_number(sub_device_number) {}

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

    uint16_t DeviceNumber() const { return m_sub_device_number; }
    uint16_t StartAddress() const { return m_start_address; }
    uint16_t Footprint() const {
      return PERSONALITIES[m_personality].footprint;
    }

    const ola::rdm::UID &UID() const { return m_uid; }

  private:
    /**
     * The RDM Operations for the DummyRDMDevice.
     */
    class DummyRDMDeviceOps : public ola::rdm::ResponderOps<DummyRDMDevice> {
      public:
        static DummyRDMDeviceOps *Instance() {
          if (!instance)
            instance = new DummyRDMDeviceOps();
          return instance;
        }

      private:
        DummyRDMDeviceOps() : ResponderOps(PARAM_HANDLERS) {}

        static DummyRDMDeviceOps *instance;
    };

    const ola::rdm::UID m_uid;
    uint16_t m_start_address;
    uint8_t m_personality;
    uint8_t m_identify_mode;
    uint32_t m_lamp_strikes;
    uint16_t m_sub_device_number;

    RDMResponse *GetParamDescription(const ola::rdm::RDMRequest *request);
    RDMResponse *GetDeviceInfo(const ola::rdm::RDMRequest *request);
    RDMResponse *GetFactoryDefaults(const ola::rdm::RDMRequest *request);
    RDMResponse *SetFactoryDefaults(const ola::rdm::RDMRequest *request);
    RDMResponse *GetProductDetailList(const ola::rdm::RDMRequest *request);
    RDMResponse *GetPersonality(const ola::rdm::RDMRequest *request);
    RDMResponse *SetPersonality(const ola::rdm::RDMRequest *request);
    RDMResponse *GetPersonalityDescription(const ola::rdm::RDMRequest *request);
    RDMResponse *GetDmxStartAddress(const ola::rdm::RDMRequest *request);
    RDMResponse *SetDmxStartAddress(const ola::rdm::RDMRequest *request);
    RDMResponse *GetLampStrikes(const ola::rdm::RDMRequest *request);
    RDMResponse *SetLampStrikes(const ola::rdm::RDMRequest *request);
    RDMResponse *GetIdentify(const ola::rdm::RDMRequest *request);
    RDMResponse *SetIdentify(const ola::rdm::RDMRequest *request);
    RDMResponse *GetRealTimeClock(const ola::rdm::RDMRequest *request);
    RDMResponse *GetManufacturerLabel(const ola::rdm::RDMRequest *request);
    RDMResponse *GetDeviceLabel(const ola::rdm::RDMRequest *request);
    RDMResponse *GetDeviceModelDescription(const ola::rdm::RDMRequest *request);
    RDMResponse *GetSoftwareVersionLabel(const ola::rdm::RDMRequest *request);
    RDMResponse *GetOlaCodeVersion(const ola::rdm::RDMRequest *request);

    RDMResponse *HandleStringResponse(
        const ola::rdm::RDMRequest *request,
        const std::string &value);

    typedef struct {
      uint16_t footprint;
      const char *description;
    } personality_info;

    static const ola::rdm::ResponderOps<DummyRDMDevice>::ParamHandler
      PARAM_HANDLERS[];
    static const personality_info PERSONALITIES[];
};
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_DUMMY_DUMMYRDMDEVICE_H_
