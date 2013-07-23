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

    struct min_level_s {
      uint16_t min_level_increasing;
      uint16_t min_level_decreasing;
      uint8_t on_below_min;
    } __attribute__((packed));

    struct preset_playback_s {
      uint16_t mode;
      uint8_t level;
    } __attribute__((packed));

    struct preset_status_s {
      uint16_t scene;
      uint16_t fade_up_time;
      uint16_t fade_down_time;
      uint16_t wait_time;
      uint8_t programmed;
    } __attribute__((packed));

    struct fail_mode_s {
      uint16_t scene;
      uint16_t delay;
      uint16_t hold_time;
      uint8_t level;
    } __attribute__((packed));

    typedef fail_mode_s startup_mode_s;

    /*
     * Represents a preset
     */
    class Preset {
      public:
        Preset()
          : fade_up_time(0),
            fade_down_time(0),
            wait_time(0),
            programmed(PRESET_NOT_PROGRAMMED) {
        }

        // Times are in 1/0ths of a second.
        uint16_t fade_up_time;
        uint16_t fade_down_time;
        uint16_t wait_time;
        rdm_preset_programmed_mode programmed;
    };

    const UID m_uid;
    bool m_identify_state;
    uint16_t m_start_address;
    uint16_t m_maximum_level;
    min_level_s m_min_level;
    uint8_t m_identify_mode;
    uint8_t m_burn_in;
    bool m_power_on_self_test;
    PersonalityManager m_personality_manager;
    BasicSettingManager m_curve_settings;
    BasicSettingManager m_response_time_settings;
    SettingManager<FrequencyModulationSetting> m_frequency_settings;
    std::vector<Preset> m_presets;
    uint16_t m_preset_scene;
    uint8_t m_preset_level;
    rdm_preset_merge_mode m_preset_merge_mode;
    fail_mode_s m_fail_mode;
    startup_mode_s m_startup_mode;

    // Helpers
    bool ValueBetweenRange(const uint16_t value,
                           const uint16_t lower,
                           const uint16_t upper);

    // Pids
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
    const RDMResponse *GetMinimumLevel(const RDMRequest *request);
    const RDMResponse *SetMinimumLevel(const RDMRequest *request);
    const RDMResponse *GetMaximumLevel(const RDMRequest *request);
    const RDMResponse *SetMaximumLevel(const RDMRequest *request);
    const RDMResponse *GetIdentify(const RDMRequest *request);
    const RDMResponse *SetIdentify(const RDMRequest *request);
    const RDMResponse *SetCapturePreset(const RDMRequest *request);
    const RDMResponse *GetPresetPlayback(const RDMRequest *request);
    const RDMResponse *SetPresetPlayback(const RDMRequest *request);
    const RDMResponse *GetPresetStatus(const RDMRequest *request);
    const RDMResponse *SetPresetStatus(const RDMRequest *request);
    const RDMResponse *GetPresetMergeMode(const RDMRequest *request);
    const RDMResponse *SetPresetMergeMode(const RDMRequest *request);
    const RDMResponse *GetFailMode(const RDMRequest *request);
    const RDMResponse *SetFailMode(const RDMRequest *request);
    const RDMResponse *GetStartUpMode(const RDMRequest *request);
    const RDMResponse *SetStartUpMode(const RDMRequest *request);
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
    static const uint16_t LOWER_MIN_LEVEL;
    static const uint16_t UPPER_MIN_LEVEL;
    static const uint16_t LOWER_MAX_LEVEL;
    static const uint16_t UPPER_MAX_LEVEL;
    static const unsigned int PRESENT_COUNT;

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
