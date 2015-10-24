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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * MovingLightResponder.h
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

#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/ResponderOps.h>
#include <ola/rdm/ResponderPersonality.h>
#include <ola/rdm/UID.h>

#include <string>

namespace ola {
namespace rdm {

/**
 * A simulated moving light.
 */
class MovingLightResponder: public RDMControllerInterface {
 public:
  explicit MovingLightResponder(const UID &uid);

  void SendRDMRequest(RDMRequest *request, RDMCallback *callback);

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
  std::string m_language;
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
  std::string m_device_label;
  PersonalityManager m_personality_manager;

  RDMResponse *GetParamDescription(const RDMRequest *request);
  RDMResponse *GetDeviceInfo(const RDMRequest *request);
  RDMResponse *GetFactoryDefaults(const RDMRequest *request);
  RDMResponse *SetFactoryDefaults(const RDMRequest *request);
  RDMResponse *GetLanguageCapabilities(const RDMRequest *request);
  RDMResponse *GetLanguage(const RDMRequest *request);
  RDMResponse *SetLanguage(const RDMRequest *request);
  RDMResponse *GetProductDetailList(const RDMRequest *request);
  RDMResponse *GetPersonality(const RDMRequest *request);
  RDMResponse *SetPersonality(const RDMRequest *request);
  RDMResponse *GetPersonalityDescription(const RDMRequest *request);
  RDMResponse *GetSlotInfo(const RDMRequest *request);
  RDMResponse *GetSlotDescription(const RDMRequest *request);
  RDMResponse *GetSlotDefaultValues(const RDMRequest *request);
  RDMResponse *GetDmxStartAddress(const RDMRequest *request);
  RDMResponse *SetDmxStartAddress(const RDMRequest *request);
  RDMResponse *GetDeviceHours(const RDMRequest *request);
  RDMResponse *SetDeviceHours(const RDMRequest *request);
  RDMResponse *GetLampHours(const RDMRequest *request);
  RDMResponse *SetLampHours(const RDMRequest *request);
  RDMResponse *GetLampStrikes(const RDMRequest *request);
  RDMResponse *SetLampStrikes(const RDMRequest *request);
  RDMResponse *GetLampState(const RDMRequest *request);
  RDMResponse *SetLampState(const RDMRequest *request);
  RDMResponse *GetLampOnMode(const RDMRequest *request);
  RDMResponse *SetLampOnMode(const RDMRequest *request);
  RDMResponse *GetDevicePowerCycles(const RDMRequest *request);
  RDMResponse *SetDevicePowerCycles(const RDMRequest *request);
  RDMResponse *GetIdentify(const RDMRequest *request);
  RDMResponse *SetIdentify(const RDMRequest *request);
  RDMResponse *GetDisplayInvert(const RDMRequest *request);
  RDMResponse *SetDisplayInvert(const RDMRequest *request);
  RDMResponse *GetDisplayLevel(const RDMRequest *request);
  RDMResponse *SetDisplayLevel(const RDMRequest *request);
  RDMResponse *GetPanInvert(const RDMRequest *request);
  RDMResponse *SetPanInvert(const RDMRequest *request);
  RDMResponse *GetTiltInvert(const RDMRequest *request);
  RDMResponse *SetTiltInvert(const RDMRequest *request);
  RDMResponse *GetPanTiltSwap(const RDMRequest *request);
  RDMResponse *SetPanTiltSwap(const RDMRequest *request);
  RDMResponse *GetRealTimeClock(const RDMRequest *request);
  RDMResponse *SetResetDevice(const RDMRequest *request);
  RDMResponse *GetPowerState(const RDMRequest *request);
  RDMResponse *SetPowerState(const RDMRequest *request);
  RDMResponse *GetManufacturerLabel(const RDMRequest *request);
  RDMResponse *GetDeviceLabel(const RDMRequest *request);
  RDMResponse *SetDeviceLabel(const RDMRequest *request);
  RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
  RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
  RDMResponse *GetOlaCodeVersion(const RDMRequest *request);

  static const ResponderOps<MovingLightResponder>::ParamHandler
    PARAM_HANDLERS[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_MOVINGLIGHTRESPONDER_H_
