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
 * MovingLightResponder_h
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file MovingLightResponder.h
 * @brief A soft RDM responder that implements the basics of a moving light
 * responder.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_MOVINGLIGHTRESPONDER_H_
#define INCLUDE_OLA_RDM_MOVINGLIGHTRESPONDER_H_

#include <string>
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderOps.h"
#include "ola/rdm/ResponderPersonality.h"
#include "ola/rdm/UID.h"

namespace ola {
namespace rdm {

/**
 * A simulated moving light.
 */
class MovingLightResponder: public RDMControllerInterface {
  public:
    explicit MovingLightResponder(const UID &uid);

    void SendRDMRequest(const RDMRequest *request, RDMCallback *callback);

    uint16_t StartAddress() const { return m_start_address; }
    uint16_t Footprint() const {
      return m_personality_manager.ActivePersonalityFootprint();
    }

  private:
    /**
     * The RDM Operations for the MovingLightResponder.
     */
    class RDMOps : public ResponderOps<MovingLightResponder> {
      public:
        static RDMOps *Instance() {
          if (!instance)
            instance = new RDMOps();
          return instance;
        }

      private:
        RDMOps() : ResponderOps<MovingLightResponder>(PARAM_HANDLERS) {}

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
    string m_language;
    bool m_identify_mode;
    bool m_pan_invert;
    bool m_tilt_invert;
    uint32_t m_device_hours;
    uint32_t m_lamp_hours;
    uint32_t m_lamp_strikes;
    rdm_lamp_state m_lamp_state;
    rdm_lamp_mode m_lamp_on_mode;
    uint32_t m_device_power_cycles;
    rdm_display_invert m_display_invert;
    uint8_t m_display_level;
    bool m_pan_tilt_swap;
    rdm_power_state m_power_state;
    string m_device_label;
    PersonalityManager m_personality_manager;

    const RDMResponse *GetParamDescription(const RDMRequest *request);
    const RDMResponse *GetDeviceInfo(const RDMRequest *request);
    const RDMResponse *GetFactoryDefaults(const RDMRequest *request);
    const RDMResponse *SetFactoryDefaults(const RDMRequest *request);
    const RDMResponse *GetLanguageCapabilities(const RDMRequest *request);
    const RDMResponse *GetLanguage(const RDMRequest *request);
    const RDMResponse *SetLanguage(const RDMRequest *request);
    const RDMResponse *GetProductDetailList(const RDMRequest *request);
    const RDMResponse *GetPersonality(const RDMRequest *request);
    const RDMResponse *SetPersonality(const RDMRequest *request);
    const RDMResponse *GetPersonalityDescription(const RDMRequest *request);
    const RDMResponse *GetSlotInfo(const RDMRequest *request);
    const RDMResponse *GetSlotDescription(const RDMRequest *request);
    const RDMResponse *GetDmxStartAddress(const RDMRequest *request);
    const RDMResponse *SetDmxStartAddress(const RDMRequest *request);
    const RDMResponse *GetDeviceHours(const RDMRequest *request);
    const RDMResponse *SetDeviceHours(const RDMRequest *request);
    const RDMResponse *GetLampHours(const RDMRequest *request);
    const RDMResponse *SetLampHours(const RDMRequest *request);
    const RDMResponse *GetLampStrikes(const RDMRequest *request);
    const RDMResponse *SetLampStrikes(const RDMRequest *request);
    const RDMResponse *GetLampState(const RDMRequest *request);
    const RDMResponse *SetLampState(const RDMRequest *request);
    const RDMResponse *GetLampOnMode(const RDMRequest *request);
    const RDMResponse *SetLampOnMode(const RDMRequest *request);
    const RDMResponse *GetDevicePowerCycles(const RDMRequest *request);
    const RDMResponse *SetDevicePowerCycles(const RDMRequest *request);
    const RDMResponse *GetIdentify(const RDMRequest *request);
    const RDMResponse *SetIdentify(const RDMRequest *request);
    const RDMResponse *GetDisplayInvert(const RDMRequest *request);
    const RDMResponse *SetDisplayInvert(const RDMRequest *request);
    const RDMResponse *GetDisplayLevel(const RDMRequest *request);
    const RDMResponse *SetDisplayLevel(const RDMRequest *request);
    const RDMResponse *GetPanInvert(const RDMRequest *request);
    const RDMResponse *SetPanInvert(const RDMRequest *request);
    const RDMResponse *GetTiltInvert(const RDMRequest *request);
    const RDMResponse *SetTiltInvert(const RDMRequest *request);
    const RDMResponse *GetPanTiltSwap(const RDMRequest *request);
    const RDMResponse *SetPanTiltSwap(const RDMRequest *request);
    const RDMResponse *GetRealTimeClock(const RDMRequest *request);
    const RDMResponse *GetPowerState(const RDMRequest *request);
    const RDMResponse *SetPowerState(const RDMRequest *request);
    const RDMResponse *GetManufacturerLabel(const RDMRequest *request);
    const RDMResponse *GetDeviceLabel(const RDMRequest *request);
    const RDMResponse *SetDeviceLabel(const RDMRequest *request);
    const RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
    const RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
    const RDMResponse *GetOlaCodeVersion(const RDMRequest *request);

    static const ResponderOps<MovingLightResponder>::ParamHandler
      PARAM_HANDLERS[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_MOVINGLIGHTRESPONDER_H_
