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
 * AdvancedDimmerResponder_h
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file AdvancedDimmerResponder.h
 * @brief Soft responder that implements a Dimmer that supports E1.37-1 PIDs
 * @}
 */

#ifndef INCLUDE_OLA_RDM_ADVANCEDDIMMERRESPONDER_H_
#define INCLUDE_OLA_RDM_ADVANCEDDIMMERRESPONDER_H_

#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/ResponderOps.h>
#include <ola/rdm/ResponderPersonality.h>
#include <ola/rdm/ResponderSettings.h>
#include <ola/rdm/UID.h>
#include <memory>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

using std::auto_ptr;

/**
 * A dimmer that supports many of the E1.37-1 PIDs.
 */
class AdvancedDimmerResponder: public RDMControllerInterface {
  public:
    explicit AdvancedDimmerResponder(const UID &uid);

    void SendRDMRequest(const RDMRequest *request, RDMCallback *callback);

  private:
    /**
     * The RDM Operations for the AdvancedDimmerResponder.
     */
    class RDMOps : public ResponderOps<AdvancedDimmerResponder> {
      public:
        static RDMOps *Instance() {
          if (!instance)
            instance = new RDMOps();
          return instance;
        }

      private:
        RDMOps() : ResponderOps<AdvancedDimmerResponder>(PARAM_HANDLERS) {}
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
    bool m_identify_state;
    uint16_t m_start_address;
    uint16_t m_maximum_level;
    uint8_t m_identify_mode;
    uint8_t m_burn_in;
    bool m_power_on_self_test;
    PersonalityManager m_personality_manager;
    BasicSettingManager m_curve_settings;
    BasicSettingManager m_response_time_settings;
    SettingManager<FrequencyModulationSetting> m_frequency_settings;

    const RDMResponse *GetDeviceInfo(const RDMRequest *request);
    const RDMResponse *GetProductDetailList(const RDMRequest *request);
    const RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
    const RDMResponse *GetManufacturerLabel(const RDMRequest *request);
    const RDMResponse *GetDeviceLabel(const RDMRequest *request);
    const RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
    const RDMResponse *GetPersonality(const RDMRequest *request);
    const RDMResponse *SetPersonality(const RDMRequest *request);
    const RDMResponse *GetPersonalityDescription(const RDMRequest *request);
    const RDMResponse *GetDmxStartAddress(const RDMRequest *request);
    const RDMResponse *SetDmxStartAddress(const RDMRequest *request);
    const RDMResponse *GetDimmerInfo(const RDMRequest *request);
    const RDMResponse *GetMaximumLevel(const RDMRequest *request);
    const RDMResponse *SetMaximumLevel(const RDMRequest *request);
    const RDMResponse *GetIdentify(const RDMRequest *request);
    const RDMResponse *SetIdentify(const RDMRequest *request);
    const RDMResponse *GetIdentifyMode(const RDMRequest *request);
    const RDMResponse *SetIdentifyMode(const RDMRequest *request);
    const RDMResponse *GetBurnIn(const RDMRequest *request);
    const RDMResponse *SetBurnIn(const RDMRequest *request);
    const RDMResponse *GetCurve(const RDMRequest *request);
    const RDMResponse *SetCurve(const RDMRequest *request);
    const RDMResponse *GetCurveDescription(const RDMRequest *request);
    const RDMResponse *GetResponseTime(const RDMRequest *request);
    const RDMResponse *SetResponseTime(const RDMRequest *request);
    const RDMResponse *GetResponseTimeDescription(const RDMRequest *request);
    const RDMResponse *GetPWMFrequency(const RDMRequest *request);
    const RDMResponse *SetPWMFrequency(const RDMRequest *request);
    const RDMResponse *GetPWMFrequencyDescription(const RDMRequest *request);
    const RDMResponse *GetPowerOnSelfTest(const RDMRequest *request);
    const RDMResponse *SetPowerOnSelfTest(const RDMRequest *request);

    static const uint8_t DIMMER_RESOLUTION;
    static const uint16_t LOWER_MAX_LEVEL;
    static const uint16_t UPPER_MAX_LEVEL;

    static const ResponderOps<AdvancedDimmerResponder>::ParamHandler
      PARAM_HANDLERS[];
    static const char* CURVES[];
    static const char* RESPONSE_TIMES[];
    static const FrequencyModulationSetting::FrequencyModulationArg
        PWM_FREQUENCIES[];

    static const SettingCollection<BasicSetting> CurveSettings;
    static const SettingCollection<BasicSetting> ResponseTimeSettings;
    static const SettingCollection<FrequencyModulationSetting>
      FrequencySettings;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_ADVANCEDDIMMERRESPONDER_H_
