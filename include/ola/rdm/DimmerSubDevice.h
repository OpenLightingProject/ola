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
 * DimmerSubDevice.h
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file DimmerSubDevice.h
 * @brief A soft responder that impements a SubDevice in DimmerResponder
 * @}
 */

#ifndef INCLUDE_OLA_RDM_DIMMERSUBDEVICE_H_
#define INCLUDE_OLA_RDM_DIMMERSUBDEVICE_H_

#include <string>
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderOps.h"
#include "ola/rdm/ResponderPersonality.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace rdm {

/**
 * A sub device in the simulated dimmer.
 */
class DimmerSubDevice: public RDMControllerInterface {
  public:
    DimmerSubDevice(const UID &uid, uint16_t sub_device_number);

    void SendRDMRequest(const RDMRequest *request, RDMCallback *callback);

    uint16_t Footprint() const {
      return m_personality_manager.ActivePersonalityFootprint();
    }

    bool SetDmxStartAddress(uint16_t start_address);

    uint16_t GetDmxStartAddress() const {
       return m_start_address;
    }

  private:
    /**
     * The RDM Operations for the DimmerSubDevice.
     */
    class RDMOps : public ResponderOps<DimmerSubDevice> {
      public:
        static RDMOps *Instance() {
          if (!instance)
            instance = new RDMOps();
          return instance;
        }

      private:
        RDMOps() : ResponderOps<DimmerSubDevice>(PARAM_HANDLERS) {}

        static RDMOps *instance;
    };

    /**
     * The personalities
     */
    class Personalities : public PersonalityCollection {
      public:
        static const Personalities *Instance();

      private:
        explicit Personalities(const PersonalityList &personalities) :
          PersonalityCollection(personalities) {
        }

        static Personalities *instance;
    };

    const UID m_uid;
    const uint16_t m_sub_device_number;
    uint16_t m_start_address;
    bool m_identify_on;
    PersonalityManager m_personality_manager;

    const RDMResponse *GetDeviceInfo(const RDMRequest *request);
    const RDMResponse *GetProductDetailList(const RDMRequest *request);
    const RDMResponse *GetPersonality(const RDMRequest *request);
    const RDMResponse *SetPersonality(const RDMRequest *request);
    const RDMResponse *GetPersonalityDescription(const RDMRequest *request);
    const RDMResponse *GetDmxStartAddress(const RDMRequest *request);
    const RDMResponse *SetDmxStartAddress(const RDMRequest *request);
    const RDMResponse *GetIdentify(const RDMRequest *request);
    const RDMResponse *SetIdentify(const RDMRequest *request);
    const RDMResponse *GetRealTimeClock(const RDMRequest *request);
    const RDMResponse *GetManufacturerLabel(const RDMRequest *request);
    const RDMResponse *GetDeviceLabel(const RDMRequest *request);
    const RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
    const RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);

    static const ResponderOps<DimmerSubDevice>::ParamHandler PARAM_HANDLERS[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_DIMMERSUBDEVICE_H_
