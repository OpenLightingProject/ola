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
 * AdvancedDimmerResponder.h
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

#include <ola/base/Macro.h>
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

/**
 * A dimmer that supports many of the E1.37-1 PIDs.
 */
class AdvancedDimmerResponder: public RDMControllerInterface {
 public:
  explicit AdvancedDimmerResponder(const UID &uid);

  void SendRDMRequest(RDMRequest *request, RDMCallback *callback);

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

  class LockManager: public BasicSettingManager {
   public:
    explicit LockManager(const SettingCollection<BasicSetting> *settings)
        : BasicSettingManager(settings) {
    }

    RDMResponse *SetWithPin(const RDMRequest *request, uint16_t pin);
  };

  PACK(
  struct min_level_s {
    uint16_t min_level_increasing;
    uint16_t min_level_decreasing;
    uint8_t on_below_min;
  });

  PACK(
  struct preset_playback_s {
    uint16_t mode;
    uint8_t level;
  });

  PACK(
  struct preset_status_s {
    uint16_t scene;
    uint16_t fade_up_time;
    uint16_t fade_down_time;
    uint16_t wait_time;
    uint8_t programmed;
  });

  PACK(
  struct fail_mode_s {
    uint16_t scene;
    uint16_t delay;
    uint16_t hold_time;
    uint8_t level;
  });

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
  uint16_t m_lock_pin;
  uint16_t m_maximum_level;
  min_level_s m_min_level;
  rdm_identify_mode m_identify_mode;
  uint8_t m_burn_in;
  bool m_power_on_self_test;
  PersonalityManager m_personality_manager;
  BasicSettingManager m_curve_settings;
  BasicSettingManager m_response_time_settings;
  LockManager m_lock_settings;
  SettingManager<FrequencyModulationSetting> m_frequency_settings;
  std::vector<Preset> m_presets;
  uint16_t m_preset_scene;
  uint8_t m_preset_level;
  rdm_preset_mergemode m_preset_mergemode;
  fail_mode_s m_fail_mode;
  startup_mode_s m_startup_mode;

  // Helpers
  bool ValueBetweenRange(const uint16_t value,
                         const uint16_t lower,
                         const uint16_t upper);

  // Pids
  RDMResponse *GetDeviceInfo(const RDMRequest *request);
  RDMResponse *GetProductDetailList(const RDMRequest *request);
  RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
  RDMResponse *GetManufacturerLabel(const RDMRequest *request);
  RDMResponse *GetDeviceLabel(const RDMRequest *request);
  RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
  RDMResponse *GetPersonality(const RDMRequest *request);
  RDMResponse *SetPersonality(const RDMRequest *request);
  RDMResponse *GetPersonalityDescription(const RDMRequest *request);
  RDMResponse *GetDmxStartAddress(const RDMRequest *request);
  RDMResponse *SetDmxStartAddress(const RDMRequest *request);
  RDMResponse *GetDimmerInfo(const RDMRequest *request);
  RDMResponse *GetMinimumLevel(const RDMRequest *request);
  RDMResponse *SetMinimumLevel(const RDMRequest *request);
  RDMResponse *GetMaximumLevel(const RDMRequest *request);
  RDMResponse *SetMaximumLevel(const RDMRequest *request);
  RDMResponse *GetIdentify(const RDMRequest *request);
  RDMResponse *SetIdentify(const RDMRequest *request);
  RDMResponse *SetCapturePreset(const RDMRequest *request);
  RDMResponse *GetPresetPlayback(const RDMRequest *request);
  RDMResponse *SetPresetPlayback(const RDMRequest *request);
  RDMResponse *GetPresetStatus(const RDMRequest *request);
  RDMResponse *SetPresetStatus(const RDMRequest *request);
  RDMResponse *GetPresetMergeMode(const RDMRequest *request);
  RDMResponse *SetPresetMergeMode(const RDMRequest *request);
  RDMResponse *GetPresetInfo(const RDMRequest *request);
  RDMResponse *GetFailMode(const RDMRequest *request);
  RDMResponse *SetFailMode(const RDMRequest *request);
  RDMResponse *GetStartUpMode(const RDMRequest *request);
  RDMResponse *SetStartUpMode(const RDMRequest *request);
  RDMResponse *GetIdentifyMode(const RDMRequest *request);
  RDMResponse *SetIdentifyMode(const RDMRequest *request);
  RDMResponse *GetBurnIn(const RDMRequest *request);
  RDMResponse *SetBurnIn(const RDMRequest *request);
  RDMResponse *GetCurve(const RDMRequest *request);
  RDMResponse *SetCurve(const RDMRequest *request);
  RDMResponse *GetCurveDescription(const RDMRequest *request);
  RDMResponse *GetResponseTime(const RDMRequest *request);
  RDMResponse *SetResponseTime(const RDMRequest *request);
  RDMResponse *GetResponseTimeDescription(const RDMRequest *request);
  RDMResponse *GetPWMFrequency(const RDMRequest *request);
  RDMResponse *SetPWMFrequency(const RDMRequest *request);
  RDMResponse *GetPWMFrequencyDescription(const RDMRequest *request);
  RDMResponse *GetLockState(const RDMRequest *request);
  RDMResponse *SetLockState(const RDMRequest *request);
  RDMResponse *GetLockStateDescription(const RDMRequest *request);
  RDMResponse *GetLockPin(const RDMRequest *request);
  RDMResponse *SetLockPin(const RDMRequest *request);
  RDMResponse *GetPowerOnSelfTest(const RDMRequest *request);
  RDMResponse *SetPowerOnSelfTest(const RDMRequest *request);

  static const uint8_t DIMMER_RESOLUTION;
  static const uint16_t LOWER_MIN_LEVEL;
  static const uint16_t UPPER_MIN_LEVEL;
  static const uint16_t LOWER_MAX_LEVEL;
  static const uint16_t UPPER_MAX_LEVEL;
  static const unsigned int PRESET_COUNT;

  static const uint16_t MIN_FAIL_DELAY_TIME;
  static const uint16_t MIN_FAIL_HOLD_TIME;
  static const uint16_t MAX_FAIL_DELAY_TIME;
  static const uint16_t MAX_FAIL_HOLD_TIME;
  static const uint16_t MIN_STARTUP_DELAY_TIME;
  static const uint16_t MIN_STARTUP_HOLD_TIME;
  static const uint16_t MAX_STARTUP_DELAY_TIME;
  static const uint16_t MAX_STARTUP_HOLD_TIME;
  static const uint16_t INFINITE_TIME;

  static const ResponderOps<AdvancedDimmerResponder>::ParamHandler
    PARAM_HANDLERS[];
  static const char* CURVES[];
  static const char* RESPONSE_TIMES[];
  static const char* LOCK_STATES[];
  static const FrequencyModulationSetting::FrequencyModulationArg
      PWM_FREQUENCIES[];

  static const SettingCollection<BasicSetting> CurveSettings;
  static const SettingCollection<BasicSetting> ResponseTimeSettings;
  static const SettingCollection<FrequencyModulationSetting>
    FrequencySettings;
  static const SettingCollection<BasicSetting> LockSettings;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_ADVANCEDDIMMERRESPONDER_H_
