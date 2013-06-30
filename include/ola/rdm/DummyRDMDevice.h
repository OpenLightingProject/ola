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

#ifndef INCLUDE_OLA_RDM_DUMMYRDMDEVICE_H_
#define INCLUDE_OLA_RDM_DUMMYRDMDEVICE_H_

#include <string>
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderOps.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace rdm {

class DummyRDMDevice: public RDMControllerInterface {
  public:
    DummyRDMDevice(const UID &uid, uint16_t sub_device_number)
        : m_uid(uid),
          m_start_address(1),
          m_personality(1),
          m_identify_mode(0),
          m_lamp_strikes(0),
          m_sub_device_number(sub_device_number) {}

    void SendRDMRequest(const RDMRequest *request, RDMCallback *callback);

    uint16_t DeviceNumber() const { return m_sub_device_number; }
    uint16_t StartAddress() const { return m_start_address; }
    uint16_t Footprint() const {
      return PERSONALITIES[m_personality].footprint;
    }

  private:
    /**
     * The RDM Operations for the DummyRDMDevice.
     */
    class RDMOps : public ResponderOps<DummyRDMDevice> {
      public:
        static RDMOps *Instance() {
          if (!instance)
            instance = new RDMOps();
          return instance;
        }

      private:
        RDMOps() : ResponderOps(PARAM_HANDLERS) {}

        static RDMOps *instance;
    };

    const UID m_uid;
    uint16_t m_start_address;
    uint8_t m_personality;
    uint8_t m_identify_mode;
    uint32_t m_lamp_strikes;
    uint16_t m_sub_device_number;

    RDMResponse *GetParamDescription(const RDMRequest *request);
    RDMResponse *GetDeviceInfo(const RDMRequest *request);
    RDMResponse *GetFactoryDefaults(const RDMRequest *request);
    RDMResponse *SetFactoryDefaults(const RDMRequest *request);
    RDMResponse *GetProductDetailList(const RDMRequest *request);
    RDMResponse *GetPersonality(const RDMRequest *request);
    RDMResponse *SetPersonality(const RDMRequest *request);
    RDMResponse *GetPersonalityDescription(const RDMRequest *request);
    RDMResponse *GetDmxStartAddress(const RDMRequest *request);
    RDMResponse *SetDmxStartAddress(const RDMRequest *request);
    RDMResponse *GetLampStrikes(const RDMRequest *request);
    RDMResponse *SetLampStrikes(const RDMRequest *request);
    RDMResponse *GetIdentify(const RDMRequest *request);
    RDMResponse *SetIdentify(const RDMRequest *request);
    RDMResponse *GetRealTimeClock(const RDMRequest *request);
    RDMResponse *GetManufacturerLabel(const RDMRequest *request);
    RDMResponse *GetDeviceLabel(const RDMRequest *request);
    RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
    RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
    RDMResponse *GetOlaCodeVersion(const RDMRequest *request);

    RDMResponse *HandleStringResponse(const RDMRequest *request,
                                      const std::string &value);

    typedef struct {
      uint16_t footprint;
      const char *description;
    } personality_info;

    static const ResponderOps<DummyRDMDevice>::ParamHandler
      PARAM_HANDLERS[];
    static const personality_info PERSONALITIES[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_DUMMYRDMDEVICE_H_
