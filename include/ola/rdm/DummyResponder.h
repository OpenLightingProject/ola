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
 * DummyResponder.h
 * Copyright (C) 2009 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file DummyResponder.h
 * @brief Basic soft RDM responder.
 * @}
 */
#ifndef INCLUDE_OLA_RDM_DUMMYRESPONDER_H_
#define INCLUDE_OLA_RDM_DUMMYRESPONDER_H_

#include <string>
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderOps.h"
#include "ola/rdm/ResponderPersonality.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace rdm {

class DummyResponder: public RDMControllerInterface {
  public:
    explicit DummyResponder(const UID &uid);

    void SendRDMRequest(const RDMRequest *request, RDMCallback *callback);

    uint16_t StartAddress() const { return m_start_address; }
    uint16_t Footprint() const {
      return m_personality_manager.ActivePersonalityFootprint();
    }

  private:
    /**
     * The RDM Operations for the DummyResponder.
     */
    class RDMOps : public ResponderOps<DummyResponder> {
      public:
        static RDMOps *Instance() {
          if (!instance)
            instance = new RDMOps();
          return instance;
        }

      private:
        RDMOps() : ResponderOps<DummyResponder>(PARAM_HANDLERS) {}

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
    uint16_t m_start_address;
    bool m_identify_mode;
    uint32_t m_lamp_strikes;
    PersonalityManager m_personality_manager;

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

    static const ResponderOps<DummyResponder>::ParamHandler PARAM_HANDLERS[];
    static const uint8_t DEFAULT_PERSONALITY = 2;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_DUMMYRESPONDER_H_
