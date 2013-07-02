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
        RDMOps() : ResponderOps<DummyRDMDevice>(PARAM_HANDLERS) {}

        static RDMOps *instance;
    };

    const UID m_uid;
    uint16_t m_start_address;
    uint8_t m_personality;
    bool m_identify_mode;
    uint32_t m_lamp_strikes;
    uint16_t m_sub_device_number;

    const RDMResponse *GetParamDescription(const RDMRequest *request);
    const RDMResponse *GetDeviceInfo(const RDMRequest *request);
    const RDMResponse *GetFactoryDefaults(const RDMRequest *request);
    const RDMResponse *SetFactoryDefaults(const RDMRequest *request);
    const RDMResponse *GetProductDetailList(const RDMRequest *request);
    const RDMResponse *GetPersonality(const RDMRequest *request);
    const RDMResponse *SetPersonality(const RDMRequest *request);
    const RDMResponse *GetPersonalityDescription(const RDMRequest *request);
    const RDMResponse *GetDmxStartAddress(const RDMRequest *request);
    const RDMResponse *SetDmxStartAddress(const RDMRequest *request);
    const RDMResponse *GetLampStrikes(const RDMRequest *request);
    const RDMResponse *SetLampStrikes(const RDMRequest *request);
    const RDMResponse *GetIdentify(const RDMRequest *request);
    const RDMResponse *SetIdentify(const RDMRequest *request);
    const RDMResponse *GetRealTimeClock(const RDMRequest *request);
    const RDMResponse *GetManufacturerLabel(const RDMRequest *request);
    const RDMResponse *GetDeviceLabel(const RDMRequest *request);
    const RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
    const RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
    const RDMResponse *GetOlaCodeVersion(const RDMRequest *request);

    typedef struct {
      uint16_t footprint;
      const char *description;
    } personality_info;

    static const ResponderOps<DummyRDMDevice>::ParamHandler PARAM_HANDLERS[];
    static const personality_info PERSONALITIES[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_DUMMYRDMDEVICE_H_
