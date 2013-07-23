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
 * AdvancedDimmerResponder.cpp
 * Copyright (C) 2013 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
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
using std::string;
using std::vector;

const uint8_t AdvancedDimmerResponder::DIMMER_RESOLUTION = 14;
const uint16_t AdvancedDimmerResponder::LOWER_MAX_LEVEL = 0x7fff;
const uint16_t AdvancedDimmerResponder::UPPER_MAX_LEVEL = 0xffff;
const uint16_t AdvancedDimmerResponder::LOWER_MIN_LEVEL = 0x0;
const uint16_t AdvancedDimmerResponder::UPPER_MIN_LEVEL = 0x7fff;
const unsigned int AdvancedDimmerResponder::PRESENT_COUNT = 6;

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

const SettingCollection<BasicSetting>
    AdvancedDimmerResponder::CurveSettings(CURVES, arraysize(CURVES));
const SettingCollection<BasicSetting>
    AdvancedDimmerResponder::ResponseTimeSettings(
        RESPONSE_TIMES, arraysize(RESPONSE_TIMES));
const SettingCollection<FrequencyModulationSetting>
    AdvancedDimmerResponder::FrequencySettings(
        PWM_FREQUENCIES, arraysize(PWM_FREQUENCIES));

const AdvancedDimmerResponder::Personalities *
    AdvancedDimmerResponder::Personalities::Instance() {
  if (!instance) {
    PersonalityList personalities;
    personalities.push_back(new Personality(12, "6-Channel 16-bit"));
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
  { PID_CAPTURE_PRESET,
    NULL,
    &AdvancedDimmerResponder::SetCapturePreset},
  { PID_DIMMER_INFO,
    &AdvancedDimmerResponder::GetDimmerInfo,
    NULL},
  { PID_MINIMUM_LEVEL,
    &AdvancedDimmerResponder::GetMinimumLevel,
    &AdvancedDimmerResponder::SetMinimumLevel},
  { PID_MAXIMUM_LEVEL,
    &AdvancedDimmerResponder::GetMaximumLevel,
    &AdvancedDimmerResponder::SetMaximumLevel},
  { PID_IDENTIFY_MODE,
    &AdvancedDimmerResponder::GetIdentifyMode,
    &AdvancedDimmerResponder::SetIdentifyMode},
  { PID_BURN_IN,
    &AdvancedDimmerResponder::GetBurnIn,
    &AdvancedDimmerResponder::SetBurnIn},
  { PID_CURVE,
    &AdvancedDimmerResponder::GetCurve,
    &AdvancedDimmerResponder::SetCurve},
  { PID_CURVE_DESCRIPTION,
    &AdvancedDimmerResponder::GetCurveDescription,
    NULL},
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
  { PID_POWER_ON_SELF_TEST,
    &AdvancedDimmerResponder::GetPowerOnSelfTest,
    &AdvancedDimmerResponder::SetPowerOnSelfTest},
  { 0, NULL, NULL},
};

/**
 * Create a new dimmer root device. Ownership of the DimmerSubDevices is not
 * transferred.
 */
AdvancedDimmerResponder::AdvancedDimmerResponder(const UID &uid)
    : m_uid(uid),
      m_identify_state(false),
      m_start_address(1),
      m_maximum_level(UPPER_MAX_LEVEL),
      m_identify_mode(IDENTIFY_MODE_QUIET),
      m_burn_in(0),
      m_power_on_self_test(true),
      m_personality_manager(Personalities::Instance()),
      m_curve_settings(&CurveSettings),
      m_response_time_settings(&ResponseTimeSettings),
      m_frequency_settings(&FrequencySettings),
      m_presets(PRESENT_COUNT) {
  m_min_level.min_level_increasing = 10;
  m_min_level.min_level_decreasing = 20;
  m_min_level.on_below_min = true;
}

/*
 * Handle an RDM Request
 */
void AdvancedDimmerResponder::SendRDMRequest(const RDMRequest *request,
                                             RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}
const RDMResponse *AdvancedDimmerResponder::GetDeviceInfo(
    const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_E137_DIMMER_MODEL,
      PRODUCT_CATEGORY_DIMMER, 1,
      &m_personality_manager,
      m_start_address,
      0, 0);
}

const RDMResponse *AdvancedDimmerResponder::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(request,
    vector<rdm_product_detail>(1, PRODUCT_DETAIL_TEST));
}

const RDMResponse *AdvancedDimmerResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA E1.37-1 Dimmer");
}

const RDMResponse *AdvancedDimmerResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

const RDMResponse *AdvancedDimmerResponder::GetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy Dimmer");
}

const RDMResponse *AdvancedDimmerResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

const RDMResponse *AdvancedDimmerResponder::GetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonality(request, &m_personality_manager);
}

const RDMResponse *AdvancedDimmerResponder::SetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::SetPersonality(request, &m_personality_manager,
                                         m_start_address);
}

const RDMResponse *AdvancedDimmerResponder::GetPersonalityDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonalityDescription(
      request, &m_personality_manager);
}

const RDMResponse *AdvancedDimmerResponder::GetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::GetDmxAddress(request, &m_personality_manager,
                                        m_start_address);
}

const RDMResponse *AdvancedDimmerResponder::SetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::SetDmxAddress(request, &m_personality_manager,
                                        &m_start_address);
}

const RDMResponse *AdvancedDimmerResponder::GetDimmerInfo(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  struct dimmer_info_s {
    uint16_t min_level_lower;
    uint16_t min_level_upper;
    uint16_t max_level_lower;
    uint16_t max_level_upper;
    uint8_t curve_count;
    uint8_t level_resolution;
    uint8_t level_support;
  } __attribute__((packed));

  struct dimmer_info_s dimmer_info;
  dimmer_info.min_level_lower = HostToNetwork(static_cast<uint16_t>(0));
  dimmer_info.min_level_upper = HostToNetwork(static_cast<uint16_t>(0));
  dimmer_info.max_level_lower =
      HostToNetwork(static_cast<uint16_t>(LOWER_MAX_LEVEL));
  dimmer_info.max_level_upper =
      HostToNetwork(static_cast<uint16_t>(UPPER_MAX_LEVEL));
  dimmer_info.curve_count = CurveSettings.Count();
  dimmer_info.level_resolution = DIMMER_RESOLUTION;
  dimmer_info.level_support = 1;

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&dimmer_info),
      sizeof(dimmer_info),
      RDM_ACK);
}

const RDMResponse *AdvancedDimmerResponder::GetMinimumLevel(
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

bool AdvancedDimmerResponder::CheckMinLevelRange(
    AdvancedDimmerResponder::min_level_s *newargs,
    uint16_t lower_min,
    uint16_t upper_min) {
  if (newargs->min_level_decreasing < lower_min ||
      newargs->min_level_decreasing > upper_min ||
      newargs->min_level_increasing < lower_min ||
      newargs->min_level_increasing > upper_min ||
      newargs->on_below_min > 2) {
    return false;
  }
  return true;
}

const RDMResponse *AdvancedDimmerResponder::SetMinimumLevel(
    const RDMRequest *request) {
  min_level_s args;
  if (request->ParamDataSize() != sizeof(args)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&args), request->ParamData(),
         sizeof(args));

  args.min_level_increasing = NetworkToHost(args.min_level_increasing);
  args.min_level_decreasing = NetworkToHost(args.min_level_decreasing);

  if (!CheckMinLevelRange(&args, LOWER_MIN_LEVEL, UPPER_MIN_LEVEL)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else {
    m_min_level = args;
    return ResponderHelper::EmptySetResponse(request);
  }
}

const RDMResponse *AdvancedDimmerResponder::GetMaximumLevel(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt16Value(request, m_maximum_level);
}

bool AdvancedDimmerResponder::CheckMaxLevelRange(
    uint16_t new_max,
    uint16_t lower_max,
    uint16_t upper_max) {
  if (new_max < lower_max || new_max > upper_max) {
    return false;
  }
  return true;
}

const RDMResponse *AdvancedDimmerResponder::SetMaximumLevel(
    const RDMRequest *request) {
  uint16_t arg;
  if (!ResponderHelper::ExtractUInt16(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  /*
  if (arg < LOWER_MAX_LEVEL || arg > UPPER_MAX_LEVEL) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else {
    m_maximum_level = arg;
    return ResponderHelper::EmptySetResponse(request);
  }*/

  if (!CheckMaxLevelRange(arg, LOWER_MAX_LEVEL, UPPER_MAX_LEVEL)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else {
    m_maximum_level = arg;
    return ResponderHelper::EmptySetResponse(request);
  }
}

const RDMResponse *AdvancedDimmerResponder::GetIdentify(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_state);
}

const RDMResponse *AdvancedDimmerResponder::SetIdentify(
    const RDMRequest *request) {
  bool old_value = m_identify_state;
  const RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_state);
  if (m_identify_state != old_value) {
    OLA_INFO << "E1.37-1 Dimmer Device " << m_uid << ", identify state "
             << (m_identify_state ? "on" : "off");
  }
  return response;
}

const RDMResponse *AdvancedDimmerResponder::SetCapturePreset(
    const RDMRequest *request) {
  struct preset_s {
    uint16_t scene;
    uint16_t fade_up_time;
    uint16_t fade_down_time;
    uint16_t wait_time;
  } __attribute__((packed));

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

  Preset &preset = m_presets[args.scene];
  preset.fade_up_time = args.fade_up_time;
  preset.fade_down_time = args.fade_down_time;
  preset.wait_time = args.wait_time;
  preset.programmed = true;
  return ResponderHelper::EmptySetResponse(request);
}

const RDMResponse *AdvancedDimmerResponder::GetIdentifyMode(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt8Value(request, m_identify_mode);
}

const RDMResponse *AdvancedDimmerResponder::SetIdentifyMode(
    const RDMRequest *request) {
  uint8_t arg;
  if (!ResponderHelper::ExtractUInt8(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (arg == IDENTIFY_MODE_QUIET || arg == IDENTIFY_MODE_LOUD) {
    m_identify_mode = arg;
    return ResponderHelper::EmptySetResponse(request);
  } else {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }
}

const RDMResponse *AdvancedDimmerResponder::GetBurnIn(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt8Value(request, m_burn_in);
}

const RDMResponse *AdvancedDimmerResponder::SetBurnIn(
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

const RDMResponse *AdvancedDimmerResponder::GetCurve(
    const RDMRequest *request) {
  return m_curve_settings.Get(request);
}

const RDMResponse *AdvancedDimmerResponder::SetCurve(
    const RDMRequest *request) {
  return m_curve_settings.Set(request);
}

const RDMResponse *AdvancedDimmerResponder::GetCurveDescription(
    const RDMRequest *request) {
  return m_curve_settings.GetDescription(request);
}

const RDMResponse *AdvancedDimmerResponder::GetResponseTime(
    const RDMRequest *request) {
  return m_response_time_settings.Get(request);
}

const RDMResponse *AdvancedDimmerResponder::SetResponseTime(
    const RDMRequest *request) {
  return m_response_time_settings.Set(request);
}

const RDMResponse *AdvancedDimmerResponder::GetResponseTimeDescription(
    const RDMRequest *request) {
  return m_response_time_settings.GetDescription(request);
}

const RDMResponse *AdvancedDimmerResponder::GetPWMFrequency(
    const RDMRequest *request) {
  return m_frequency_settings.Get(request);
}

const RDMResponse *AdvancedDimmerResponder::SetPWMFrequency(
    const RDMRequest *request) {
  return m_frequency_settings.Set(request);
}

const RDMResponse *AdvancedDimmerResponder::GetPWMFrequencyDescription(
    const RDMRequest *request) {
  return m_frequency_settings.GetDescription(request);
}

const RDMResponse *AdvancedDimmerResponder::GetPowerOnSelfTest(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_power_on_self_test);
}

const RDMResponse *AdvancedDimmerResponder::SetPowerOnSelfTest(
    const RDMRequest *request) {
  return ResponderHelper::SetBoolValue(request, &m_power_on_self_test);
}
}  // namespace rdm
}  // namespace ola
