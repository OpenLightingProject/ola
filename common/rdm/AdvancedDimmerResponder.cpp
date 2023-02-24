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
 * AdvancedDimmerResponder.cpp
 * Copyright (C) 2013 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/AdvancedDimmerResponder.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/ResponderSettings.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::max;
using std::min;
using std::string;
using std::vector;

const uint8_t AdvancedDimmerResponder::DIMMER_RESOLUTION = 14;
const uint16_t AdvancedDimmerResponder::LOWER_MAX_LEVEL = 0x7fff;
const uint16_t AdvancedDimmerResponder::UPPER_MAX_LEVEL = 0xffff;
const uint16_t AdvancedDimmerResponder::LOWER_MIN_LEVEL = 0x0;
const uint16_t AdvancedDimmerResponder::UPPER_MIN_LEVEL = 0x7fff;
const unsigned int AdvancedDimmerResponder::PRESET_COUNT = 6;

const uint16_t AdvancedDimmerResponder::MIN_FAIL_DELAY_TIME = 10;
const uint16_t AdvancedDimmerResponder::MIN_FAIL_HOLD_TIME = 0;
const uint16_t AdvancedDimmerResponder::MAX_FAIL_DELAY_TIME = 0x00ff;
const uint16_t AdvancedDimmerResponder::MAX_FAIL_HOLD_TIME = 0xff00;
const uint16_t AdvancedDimmerResponder::MIN_STARTUP_DELAY_TIME = 0;
const uint16_t AdvancedDimmerResponder::MIN_STARTUP_HOLD_TIME = 0;
const uint16_t AdvancedDimmerResponder::MAX_STARTUP_DELAY_TIME = 1200;
const uint16_t AdvancedDimmerResponder::MAX_STARTUP_HOLD_TIME = 36000;
const uint16_t AdvancedDimmerResponder::INFINITE_TIME = 0xffff;

const char* AdvancedDimmerResponder::CURVES[] = {
  "Linear Curve",
  "Square Law Curve",
  "S Curve",
};

const char* AdvancedDimmerResponder::RESPONSE_TIMES[] = {
  "Super fast",
  "Fast",
  "Slow",
  "Very slow",
};

const FrequencyModulationSetting::ArgType
    AdvancedDimmerResponder::PWM_FREQUENCIES[] = {
  {120, "120Hz"},
  {500, "500Hz"},
  {1000, "1kHz"},
  {5000, "5kHz"},
  {10000, "10kHz"},
};

const char* AdvancedDimmerResponder::LOCK_STATES[] = {
  "Unlocked",
  "Start Address Locked",
  "Address and Personalities Locked",
};

const SettingCollection<BasicSetting>
    AdvancedDimmerResponder::CurveSettings(CURVES, arraysize(CURVES));
const SettingCollection<BasicSetting>
    AdvancedDimmerResponder::ResponseTimeSettings(
        RESPONSE_TIMES, arraysize(RESPONSE_TIMES));
const SettingCollection<FrequencyModulationSetting>
    AdvancedDimmerResponder::FrequencySettings(
        PWM_FREQUENCIES, arraysize(PWM_FREQUENCIES));
const SettingCollection<BasicSetting>
    AdvancedDimmerResponder::LockSettings(
        LOCK_STATES, arraysize(LOCK_STATES), true);

RDMResponse *AdvancedDimmerResponder::
    LockManager::SetWithPin(const RDMRequest *request, uint16_t pin) {
  PACK(
  struct lock_s {
    uint16_t pin;
    uint8_t state;
  });
  STATIC_ASSERT(sizeof(lock_s) == 3);

  lock_s data;

  if (request->ParamDataSize() != sizeof(lock_s)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&data), request->ParamData(), sizeof(data));

  data.pin = NetworkToHost(data.pin);

  if (data.pin != pin) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  if (!ChangeSetting(data.state)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  return ResponderHelper::EmptySetResponse(request);
}

const AdvancedDimmerResponder::Personalities *
    AdvancedDimmerResponder::Personalities::Instance() {
  if (!instance) {
    PersonalityList personalities;
    personalities.push_back(Personality(12, "6-Channel 16-bit"));
    instance = new Personalities(personalities);
  }
  return instance;
}

AdvancedDimmerResponder::Personalities *
    AdvancedDimmerResponder::Personalities::instance = NULL;

AdvancedDimmerResponder::RDMOps *
    AdvancedDimmerResponder::RDMOps::instance = NULL;

const ResponderOps<AdvancedDimmerResponder>::ParamHandler
    AdvancedDimmerResponder::PARAM_HANDLERS[] = {
  { PID_DEVICE_INFO,
    &AdvancedDimmerResponder::GetDeviceInfo,
    NULL},
  { PID_PRODUCT_DETAIL_ID_LIST,
    &AdvancedDimmerResponder::GetProductDetailList,
    NULL},
  { PID_DEVICE_MODEL_DESCRIPTION,
    &AdvancedDimmerResponder::GetDeviceModelDescription,
    NULL},
  { PID_MANUFACTURER_LABEL,
    &AdvancedDimmerResponder::GetManufacturerLabel,
    NULL},
  { PID_DEVICE_LABEL,
    &AdvancedDimmerResponder::GetDeviceLabel,
    NULL},
  { PID_SOFTWARE_VERSION_LABEL,
    &AdvancedDimmerResponder::GetSoftwareVersionLabel,
    NULL},
  { PID_DMX_PERSONALITY,
    &AdvancedDimmerResponder::GetPersonality,
    &AdvancedDimmerResponder::SetPersonality},
  { PID_DMX_PERSONALITY_DESCRIPTION,
    &AdvancedDimmerResponder::GetPersonalityDescription,
    NULL},
  { PID_DMX_START_ADDRESS,
    &AdvancedDimmerResponder::GetDmxStartAddress,
    &AdvancedDimmerResponder::SetDmxStartAddress},
  { PID_IDENTIFY_DEVICE,
    &AdvancedDimmerResponder::GetIdentify,
    &AdvancedDimmerResponder::SetIdentify},
  { PID_IDENTIFY_MODE,
    &AdvancedDimmerResponder::GetIdentifyMode,
    &AdvancedDimmerResponder::SetIdentifyMode},
  { PID_CAPTURE_PRESET,
    NULL,
    &AdvancedDimmerResponder::SetCapturePreset},
  { PID_PRESET_PLAYBACK,
    &AdvancedDimmerResponder::GetPresetPlayback,
    &AdvancedDimmerResponder::SetPresetPlayback},
  { PID_DIMMER_INFO,
    &AdvancedDimmerResponder::GetDimmerInfo,
    NULL},
  { PID_MINIMUM_LEVEL,
    &AdvancedDimmerResponder::GetMinimumLevel,
    &AdvancedDimmerResponder::SetMinimumLevel},
  { PID_MAXIMUM_LEVEL,
    &AdvancedDimmerResponder::GetMaximumLevel,
    &AdvancedDimmerResponder::SetMaximumLevel},
  { PID_DMX_FAIL_MODE,
    &AdvancedDimmerResponder::GetFailMode,
    &AdvancedDimmerResponder::SetFailMode},
  { PID_DMX_STARTUP_MODE,
    &AdvancedDimmerResponder::GetStartUpMode,
    &AdvancedDimmerResponder::SetStartUpMode},
  { PID_BURN_IN,
    &AdvancedDimmerResponder::GetBurnIn,
    &AdvancedDimmerResponder::SetBurnIn},
  { PID_CURVE,
    &AdvancedDimmerResponder::GetCurve,
    &AdvancedDimmerResponder::SetCurve},
  { PID_CURVE_DESCRIPTION,
    &AdvancedDimmerResponder::GetCurveDescription,
    NULL},
  { PID_OUTPUT_RESPONSE_TIME,
    &AdvancedDimmerResponder::GetResponseTime,
    &AdvancedDimmerResponder::SetResponseTime},
  { PID_OUTPUT_RESPONSE_TIME_DESCRIPTION,
    &AdvancedDimmerResponder::GetResponseTimeDescription,
    NULL},
  { PID_MODULATION_FREQUENCY,
    &AdvancedDimmerResponder::GetPWMFrequency,
    &AdvancedDimmerResponder::SetPWMFrequency},
  { PID_MODULATION_FREQUENCY_DESCRIPTION,
    &AdvancedDimmerResponder::GetPWMFrequencyDescription,
    NULL},
  { PID_LOCK_STATE,
    &AdvancedDimmerResponder::GetLockState,
    &AdvancedDimmerResponder::SetLockState},
  { PID_LOCK_STATE_DESCRIPTION,
    &AdvancedDimmerResponder::GetLockStateDescription,
    NULL},
  { PID_LOCK_PIN,
    &AdvancedDimmerResponder::GetLockPin,
    &AdvancedDimmerResponder::SetLockPin},
  { PID_POWER_ON_SELF_TEST,
    &AdvancedDimmerResponder::GetPowerOnSelfTest,
    &AdvancedDimmerResponder::SetPowerOnSelfTest},
  { PID_PRESET_STATUS,
    &AdvancedDimmerResponder::GetPresetStatus,
    &AdvancedDimmerResponder::SetPresetStatus},
  { PID_PRESET_MERGEMODE,
    &AdvancedDimmerResponder::GetPresetMergeMode,
    &AdvancedDimmerResponder::SetPresetMergeMode},
  { PID_PRESET_INFO,
    &AdvancedDimmerResponder::GetPresetInfo,
    NULL},
  { 0, NULL, NULL},
};

/**
 * Create a new dimmer device.
 */
AdvancedDimmerResponder::AdvancedDimmerResponder(const UID &uid)
    : m_uid(uid),
      m_identify_state(false),
      m_start_address(1),
      m_lock_pin(0),
      m_maximum_level(UPPER_MAX_LEVEL),
      m_identify_mode(IDENTIFY_MODE_QUIET),
      m_burn_in(0),
      m_power_on_self_test(true),
      m_personality_manager(Personalities::Instance()),
      m_curve_settings(&CurveSettings),
      m_response_time_settings(&ResponseTimeSettings),
      m_lock_settings(&LockSettings),
      m_frequency_settings(&FrequencySettings),
      m_presets(PRESET_COUNT),
      m_preset_scene(0),
      m_preset_level(0),
      m_preset_mergemode(MERGEMODE_DEFAULT) {
  m_min_level.min_level_increasing = 10;
  m_min_level.min_level_decreasing = 20;
  m_min_level.on_below_min = true;

  m_fail_mode.scene = 0;
  m_fail_mode.delay = MIN_FAIL_DELAY_TIME;
  m_fail_mode.hold_time = MIN_FAIL_HOLD_TIME;
  m_fail_mode.level = 0;
  m_startup_mode.scene = 0;
  m_startup_mode.delay = MIN_STARTUP_DELAY_TIME;
  m_startup_mode.hold_time = MIN_STARTUP_HOLD_TIME;
  m_startup_mode.level = 255;

  // make the first preset read only
  m_presets[0].programmed = PRESET_PROGRAMMED_READ_ONLY;
}

/*
 * Handle an RDM Request
 */
void AdvancedDimmerResponder::SendRDMRequest(RDMRequest *request,
                                             RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}
RDMResponse *AdvancedDimmerResponder::GetDeviceInfo(
    const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_E137_DIMMER_MODEL,
      PRODUCT_CATEGORY_DIMMER, 1,
      &m_personality_manager,
      m_start_address,
      0, 0);
}

RDMResponse *AdvancedDimmerResponder::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(request,
    vector<rdm_product_detail>(1, PRODUCT_DETAIL_TEST));
}

RDMResponse *AdvancedDimmerResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA E1.37-1 Dimmer");
}

RDMResponse *AdvancedDimmerResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

RDMResponse *AdvancedDimmerResponder::GetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy Adv Dimmer");
}

RDMResponse *AdvancedDimmerResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

RDMResponse *AdvancedDimmerResponder::GetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonality(request, &m_personality_manager);
}

RDMResponse *AdvancedDimmerResponder::SetPersonality(
    const RDMRequest *request) {
  if (m_lock_settings.CurrentSetting() > 1) {
    return NackWithReason(request, NR_WRITE_PROTECT);
  }

  return ResponderHelper::SetPersonality(request, &m_personality_manager,
                                         m_start_address);
}

RDMResponse *AdvancedDimmerResponder::GetPersonalityDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonalityDescription(
      request, &m_personality_manager);
}

RDMResponse *AdvancedDimmerResponder::GetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::GetDmxAddress(request, &m_personality_manager,
                                        m_start_address);
}

RDMResponse *AdvancedDimmerResponder::SetDmxStartAddress(
    const RDMRequest *request) {
  if (m_lock_settings.CurrentSetting() > 0) {
    return NackWithReason(request, NR_WRITE_PROTECT);
  }

  return ResponderHelper::SetDmxAddress(request, &m_personality_manager,
                                        &m_start_address);
}

RDMResponse *AdvancedDimmerResponder::GetDimmerInfo(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  PACK(
  struct dimmer_info_s {
    uint16_t min_level_lower;
    uint16_t min_level_upper;
    uint16_t max_level_lower;
    uint16_t max_level_upper;
    uint8_t curve_count;
    uint8_t level_resolution;
    uint8_t level_support;
  });
  STATIC_ASSERT(sizeof(dimmer_info_s) == 11);

  struct dimmer_info_s dimmer_info;
  dimmer_info.min_level_lower = HostToNetwork(LOWER_MIN_LEVEL);
  dimmer_info.min_level_upper = HostToNetwork(UPPER_MIN_LEVEL);
  dimmer_info.max_level_lower = HostToNetwork(LOWER_MAX_LEVEL);
  dimmer_info.max_level_upper = HostToNetwork(UPPER_MAX_LEVEL);
  dimmer_info.curve_count = CurveSettings.Count();
  dimmer_info.level_resolution = DIMMER_RESOLUTION;
  dimmer_info.level_support = 1;

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&dimmer_info),
      sizeof(dimmer_info),
      RDM_ACK);
}

RDMResponse *AdvancedDimmerResponder::GetMinimumLevel(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  min_level_s output = m_min_level;
  output.min_level_increasing = HostToNetwork(output.min_level_increasing);
  output.min_level_decreasing = HostToNetwork(output.min_level_decreasing);

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&output),
      sizeof(output),
      RDM_ACK);
}

RDMResponse *AdvancedDimmerResponder::SetMinimumLevel(
    const RDMRequest *request) {
  min_level_s args;
  if (request->ParamDataSize() != sizeof(args)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&args), request->ParamData(),
         sizeof(args));

  args.min_level_increasing = NetworkToHost(args.min_level_increasing);
  args.min_level_decreasing = NetworkToHost(args.min_level_decreasing);

  if (!ValueBetweenRange(args.min_level_decreasing,
                         LOWER_MIN_LEVEL,
                         UPPER_MIN_LEVEL)  ||
      !ValueBetweenRange(args.min_level_increasing,
                         LOWER_MIN_LEVEL,
                         UPPER_MIN_LEVEL) ||
      args.on_below_min > 1) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else {
    m_min_level = args;
    return ResponderHelper::EmptySetResponse(request);
  }
}

RDMResponse *AdvancedDimmerResponder::GetMaximumLevel(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt16Value(request, m_maximum_level);
}

RDMResponse *AdvancedDimmerResponder::SetMaximumLevel(
    const RDMRequest *request) {
  uint16_t arg;
  if (!ResponderHelper::ExtractUInt16(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (!ValueBetweenRange(arg, LOWER_MAX_LEVEL, UPPER_MAX_LEVEL)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else {
    m_maximum_level = arg;
    return ResponderHelper::EmptySetResponse(request);
  }
}

RDMResponse *AdvancedDimmerResponder::GetIdentify(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_state);
}

RDMResponse *AdvancedDimmerResponder::SetIdentify(
    const RDMRequest *request) {
  bool old_value = m_identify_state;
  RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_state);
  if (m_identify_state != old_value) {
    OLA_INFO << "E1.37-1 Dimmer Device " << m_uid << ", identify state "
             << (m_identify_state ? "on" : "off");
  }
  return response;
}

RDMResponse *AdvancedDimmerResponder::SetCapturePreset(
    const RDMRequest *request) {
  PACK(
  struct preset_s {
    uint16_t scene;
    uint16_t fade_up_time;
    uint16_t fade_down_time;
    uint16_t wait_time;
  });
  STATIC_ASSERT(sizeof(preset_s) == 8);

  preset_s args;

  if (request->ParamDataSize() != sizeof(args)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&args), request->ParamData(),
         sizeof(args));

  args.scene = NetworkToHost(args.scene);
  args.fade_up_time = NetworkToHost(args.fade_up_time);
  args.fade_down_time = NetworkToHost(args.fade_down_time);
  args.wait_time = NetworkToHost(args.wait_time);

  if (args.scene == 0 || args.scene >= m_presets.size()) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  Preset &preset = m_presets[args.scene - 1];

  if (preset.programmed == PRESET_PROGRAMMED_READ_ONLY) {
    return NackWithReason(request, NR_WRITE_PROTECT);
  }

  preset.fade_up_time = args.fade_up_time;
  preset.fade_down_time = args.fade_down_time;
  preset.wait_time = args.wait_time;
  preset.programmed = PRESET_PROGRAMMED;
  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *AdvancedDimmerResponder::GetPresetPlayback(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  preset_playback_s output;
  output.mode = HostToNetwork(m_preset_scene);
  output.level = m_preset_level;

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&output),
      sizeof(output),
      RDM_ACK);
}

RDMResponse *AdvancedDimmerResponder::SetPresetPlayback(
    const RDMRequest *request) {
  preset_playback_s args;

  if (request->ParamDataSize() != sizeof(args)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&args), request->ParamData(),
         sizeof(args));

  args.mode = NetworkToHost(args.mode);

  if (args.mode >= m_presets.size() &&
      args.mode != PRESET_PLAYBACK_ALL) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_preset_scene = args.mode;
  m_preset_level = args.level;
  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *AdvancedDimmerResponder::GetIdentifyMode(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt8Value(request, m_identify_mode);
}

RDMResponse *AdvancedDimmerResponder::SetIdentifyMode(
    const RDMRequest *request) {
  uint8_t arg;
  if (!ResponderHelper::ExtractUInt8(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (arg == static_cast<uint8_t>(IDENTIFY_MODE_QUIET) ||
      arg == static_cast<uint8_t>(IDENTIFY_MODE_LOUD)) {
    m_identify_mode = static_cast<rdm_identify_mode>(arg);
    return ResponderHelper::EmptySetResponse(request);
  } else {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }
}

RDMResponse *AdvancedDimmerResponder::GetBurnIn(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt8Value(request, m_burn_in);
}

RDMResponse *AdvancedDimmerResponder::SetBurnIn(
    const RDMRequest *request) {
  uint8_t arg;
  if (!ResponderHelper::ExtractUInt8(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  // We start the 'clock' immediately, so the hours remaining is one less than
  // what was requested.
  m_burn_in = (arg ? arg - 1 : 0);
  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *AdvancedDimmerResponder::GetCurve(
    const RDMRequest *request) {
  return m_curve_settings.Get(request);
}

RDMResponse *AdvancedDimmerResponder::SetCurve(
    const RDMRequest *request) {
  return m_curve_settings.Set(request);
}

RDMResponse *AdvancedDimmerResponder::GetCurveDescription(
    const RDMRequest *request) {
  return m_curve_settings.GetDescription(request);
}

RDMResponse *AdvancedDimmerResponder::GetResponseTime(
    const RDMRequest *request) {
  return m_response_time_settings.Get(request);
}

RDMResponse *AdvancedDimmerResponder::SetResponseTime(
    const RDMRequest *request) {
  return m_response_time_settings.Set(request);
}

RDMResponse *AdvancedDimmerResponder::GetResponseTimeDescription(
    const RDMRequest *request) {
  return m_response_time_settings.GetDescription(request);
}

RDMResponse *AdvancedDimmerResponder::GetPWMFrequency(
    const RDMRequest *request) {
  return m_frequency_settings.Get(request);
}

RDMResponse *AdvancedDimmerResponder::SetPWMFrequency(
    const RDMRequest *request) {
  return m_frequency_settings.Set(request);
}

RDMResponse *AdvancedDimmerResponder::GetPWMFrequencyDescription(
    const RDMRequest *request) {
  return m_frequency_settings.GetDescription(request);
}

RDMResponse *AdvancedDimmerResponder::GetLockState(
    const RDMRequest *request) {
  return m_lock_settings.Get(request);
}

RDMResponse *AdvancedDimmerResponder::SetLockState(
    const RDMRequest *request) {
  return m_lock_settings.SetWithPin(request, m_lock_pin);
}

RDMResponse *AdvancedDimmerResponder::GetLockStateDescription(
    const RDMRequest *request) {
  return m_lock_settings.GetDescription(request);
}

RDMResponse *AdvancedDimmerResponder::GetLockPin(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt16Value(request, m_lock_pin, 0);
}

RDMResponse *AdvancedDimmerResponder::SetLockPin(
    const RDMRequest *request) {
  PACK(
  struct set_pin_s {
    uint16_t new_pin;
    uint16_t current_pin;
  });
  STATIC_ASSERT(sizeof(set_pin_s) == 4);

  set_pin_s data;

  if (request->ParamDataSize() != sizeof(data)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&data), request->ParamData(),
      sizeof(data));

  data.new_pin = NetworkToHost(data.new_pin);
  data.current_pin = NetworkToHost(data.current_pin);

  if (data.current_pin != m_lock_pin) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  if (data.new_pin > MAX_LOCK_PIN) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  m_lock_pin = data.new_pin;

  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *AdvancedDimmerResponder::GetPowerOnSelfTest(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_power_on_self_test);
}

RDMResponse *AdvancedDimmerResponder::SetPowerOnSelfTest(
    const RDMRequest *request) {
  return ResponderHelper::SetBoolValue(request, &m_power_on_self_test);
}

RDMResponse *AdvancedDimmerResponder::GetPresetStatus(
    const RDMRequest *request) {
  uint16_t arg;
  if (!ResponderHelper::ExtractUInt16(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (arg == 0 || arg > m_presets.size()) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  preset_status_s output;
  const Preset &preset = m_presets[arg - 1];
  output.scene = HostToNetwork(arg);
  output.fade_up_time = HostToNetwork(preset.fade_up_time);
  output.fade_down_time = HostToNetwork(preset.fade_down_time);
  output.wait_time = HostToNetwork(preset.wait_time);
  output.programmed = preset.programmed;

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&output),
      sizeof(output),
      RDM_ACK);
}

RDMResponse *AdvancedDimmerResponder::SetPresetStatus(
    const RDMRequest *request) {
  preset_status_s args;
  if (request->ParamDataSize() != sizeof(args)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&args), request->ParamData(),
         sizeof(args));

  uint16_t scene = NetworkToHost(args.scene);

  if (scene == 0 || scene > m_presets.size()) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  Preset &preset = m_presets[scene - 1];
  if (preset.programmed == PRESET_PROGRAMMED_READ_ONLY) {
    return NackWithReason(request, NR_WRITE_PROTECT);
  }

  if (args.programmed > 1) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  if (args.programmed == 1) {
    preset.fade_up_time = 0;
    preset.fade_down_time = 0;
    preset.wait_time = 0;
    preset.programmed = PRESET_NOT_PROGRAMMED;
  } else {
    preset.fade_up_time = NetworkToHost(args.fade_up_time);
    preset.fade_down_time = NetworkToHost(args.fade_down_time);
    preset.wait_time = NetworkToHost(args.wait_time);
    preset.programmed = PRESET_PROGRAMMED;
  }

  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *AdvancedDimmerResponder::GetPresetInfo(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  PACK(
  struct preset_info_s {
    uint8_t level_supported;
    uint8_t preset_seq_supported;
    uint8_t split_times_supported;
    uint8_t fail_infinite_delay_supported;
    uint8_t fail_infinite_hold_supported;
    uint8_t startup_infinite_hold_supported;
    uint16_t max_scene_number;
    uint16_t min_preset_fade_time;
    uint16_t max_preset_fade_time;
    uint16_t min_preset_wait_time;
    uint16_t max_preset_wait_time;
    uint16_t min_fail_delay_time;
    uint16_t max_fail_delay_time;
    uint16_t min_fail_hold_time;
    uint16_t max_fail_hold_time;
    uint16_t min_startup_delay;
    uint16_t max_startup_delay;
    uint16_t min_startup_hold;
    uint16_t max_startup_hold;
  });
  STATIC_ASSERT(sizeof(preset_info_s) == 32);

  uint16_t preset_count = m_presets.size();

  preset_info_s preset_info = {
    1,  // level_supported
    1,  // preset_seq_supported
    1,  // split_times_supported
    1,  // fail_infinite_delay_supported
    1,  // fail_infinite_hold_supported
    1,  // startup_infinite_hold_supported
    HostToNetwork(preset_count),
    0, 0xfffe,  // fade time
    0, 0xfffe,  // wait time
    HostToNetwork(MIN_FAIL_DELAY_TIME),
    HostToNetwork(MAX_FAIL_DELAY_TIME),  // fail delay
    HostToNetwork(MIN_FAIL_HOLD_TIME),
    HostToNetwork(MAX_FAIL_HOLD_TIME),  // fail hold time
    HostToNetwork(MIN_STARTUP_DELAY_TIME),
    HostToNetwork(MAX_STARTUP_DELAY_TIME),  // startup delay
    HostToNetwork(MIN_STARTUP_HOLD_TIME),
    HostToNetwork(MAX_STARTUP_HOLD_TIME),  // startup hold time
  };

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&preset_info),
      sizeof(preset_info),
      RDM_ACK);
}

RDMResponse *AdvancedDimmerResponder::GetPresetMergeMode(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint8_t output = m_preset_mergemode;
  return GetResponseFromData(request, &output, sizeof(output), RDM_ACK);
}

RDMResponse *AdvancedDimmerResponder::SetPresetMergeMode(
    const RDMRequest *request) {
  uint8_t arg;
  if (!ResponderHelper::ExtractUInt8(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (arg > MERGEMODE_DMX_ONLY) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }
  m_preset_mergemode = static_cast<rdm_preset_mergemode>(arg);
  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *AdvancedDimmerResponder::GetFailMode(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  fail_mode_s fail_mode = {
    HostToNetwork(m_fail_mode.scene),
    HostToNetwork(m_fail_mode.delay),
    HostToNetwork(m_fail_mode.hold_time),
    m_fail_mode.level
  };

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&fail_mode),
      sizeof(fail_mode),
      RDM_ACK);
}

RDMResponse *AdvancedDimmerResponder::SetFailMode(
    const RDMRequest *request) {
  fail_mode_s args;
  if (request->ParamDataSize() != sizeof(args)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&args), request->ParamData(),
         sizeof(args));

  uint16_t scene = NetworkToHost(args.scene);
  if (scene >= m_presets.size()) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_fail_mode.scene = NetworkToHost(args.scene);
  uint16_t delay = NetworkToHost(args.delay);
  if (delay == INFINITE_TIME) {
    m_fail_mode.delay = INFINITE_TIME;
  } else {
    m_fail_mode.delay = max(MIN_FAIL_DELAY_TIME,
                            min(MAX_FAIL_DELAY_TIME, delay));
  }
  uint16_t hold = NetworkToHost(args.hold_time);
  if (hold == INFINITE_TIME) {
    m_fail_mode.hold_time = INFINITE_TIME;
  } else {
    m_fail_mode.hold_time = max(MIN_FAIL_HOLD_TIME,
                                min(MAX_FAIL_HOLD_TIME, hold));
  }

  m_fail_mode.level = args.level;

  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *AdvancedDimmerResponder::GetStartUpMode(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  startup_mode_s startup_mode = {
    HostToNetwork(m_startup_mode.scene),
    HostToNetwork(m_startup_mode.delay),
    HostToNetwork(m_startup_mode.hold_time),
    m_startup_mode.level
  };

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&startup_mode),
      sizeof(startup_mode),
      RDM_ACK);
}

RDMResponse *AdvancedDimmerResponder::SetStartUpMode(
    const RDMRequest *request) {
  startup_mode_s args;
  if (request->ParamDataSize() != sizeof(args)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&args), request->ParamData(),
         sizeof(args));

  uint16_t scene = NetworkToHost(args.scene);
  if (scene >= m_presets.size()) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_startup_mode.scene = NetworkToHost(args.scene);

  uint16_t delay = NetworkToHost(args.delay);
  if (delay == INFINITE_TIME) {
    m_startup_mode.delay = INFINITE_TIME;
  } else {
    m_startup_mode.delay = max(MIN_STARTUP_DELAY_TIME,
                            min(MAX_STARTUP_DELAY_TIME, delay));
  }

  uint16_t hold = NetworkToHost(args.hold_time);
  if (hold == INFINITE_TIME) {
    m_startup_mode.hold_time = INFINITE_TIME;
  } else {
    m_startup_mode.hold_time = max(MIN_STARTUP_HOLD_TIME,
                                   min(MAX_STARTUP_HOLD_TIME, hold));
  }

  m_startup_mode.level = args.level;

  return ResponderHelper::EmptySetResponse(request);
}

bool AdvancedDimmerResponder::ValueBetweenRange(const uint16_t value,
                                                const uint16_t lower,
                                                const uint16_t upper) {
  return value >= lower && value <= upper;
}
}  // namespace rdm
}  // namespace ola
