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
 * MovingLightResponder.cpp
 * Copyright (C) 2013 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <iostream>
#include <string>
#include <vector>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/MovingLightResponder.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/RDMHelper.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/ResponderSlotData.h"
#include "ola/StringUtils.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

MovingLightResponder::RDMOps *MovingLightResponder::RDMOps::instance = NULL;

const MovingLightResponder::Personalities *
    MovingLightResponder::Personalities::Instance() {
  if (!instance) {
    SlotDataCollection::SlotDataList p1_slot_data;
    p1_slot_data.push_back(
        SlotData::PrimarySlot(SD_INTENSITY, 0, "Intensity Coarse"));  // 0
    p1_slot_data.push_back(
        SlotData::SecondarySlot(ST_SEC_FINE, 0, 0, "Intensity Fine"));  // 1
    p1_slot_data.push_back(
        SlotData::SecondarySlot(ST_SEC_CONTROL, 0, 0, "Shutter"));  // 2
    p1_slot_data.push_back(SlotData::PrimarySlot(SD_PAN, 127));  // 3
    p1_slot_data.push_back(
        SlotData::SecondarySlot(ST_SEC_SPEED, 3, 0, "Pan Speed"));  // 4
    p1_slot_data.push_back(SlotData::PrimarySlot(SD_TILT, 127));  // 5
    p1_slot_data.push_back(
        SlotData::SecondarySlot(ST_SEC_TIMING, 5, 0, "Tilt Timing"));  // 6
    p1_slot_data.push_back(SlotData::PrimarySlot(SD_ROTO_GOBO_WHEEL, 0));  // 7
    p1_slot_data.push_back(
        SlotData::SecondarySlot(ST_SEC_INDEX, 7, 0));  // 8
    p1_slot_data.push_back(SlotData::PrimarySlot(SD_PRISM_WHEEL, 0));  // 9
    p1_slot_data.push_back(
        SlotData::SecondarySlot(ST_SEC_ROTATION, 8, 0));  // 10
    p1_slot_data.push_back(SlotData::PrimarySlot(SD_EFFECTS_WHEEL, 0));  // 11
    p1_slot_data.push_back(
        SlotData::SecondarySlot(ST_SEC_INDEX_ROTATE, 8, 0));  // 12
    p1_slot_data.push_back(
        SlotData::PrimarySlot(SD_FIXTURE_SPEED, 0, "Speed"));  // 13
    p1_slot_data.push_back(
        SlotData::SecondarySlot(ST_SEC_SPEED, 13, 0, "Speed ^ 2"));  // 14
    p1_slot_data.push_back(
        SlotData::PrimarySlot(SD_UNDEFINED,
                              0,
                              "Open Sourceiness Foo"));  // 15
    p1_slot_data.push_back(
        SlotData::SecondarySlot(ST_SEC_UNDEFINED,
                                15,
                                0,
                                "Open Sourceiness Bar"));  // 16

    SlotDataCollection::SlotDataList p2_slot_data;
    p2_slot_data.push_back(SlotData::PrimarySlot(SD_INTENSITY, 0));
    p2_slot_data.push_back(SlotData::PrimarySlot(SD_PAN, 127));
    p2_slot_data.push_back(SlotData::PrimarySlot(SD_TILT, 127));
    p2_slot_data.push_back(SlotData::PrimarySlot(SD_COLOR_WHEEL, 0));
    p2_slot_data.push_back(SlotData::PrimarySlot(SD_STATIC_GOBO_WHEEL, 0));

    SlotDataCollection::SlotDataList p4_slot_data;
    p4_slot_data.push_back(
        SlotData::PrimarySlot(SD_INTENSITY, 0, ""));
    p4_slot_data.push_back(
        SlotData::SecondarySlot(ST_SEC_FINE, 0, 0, ""));

    PersonalityList personalities;
    personalities.push_back(Personality(17,
                                        "Full",
                                        SlotDataCollection(p1_slot_data)));
    personalities.push_back(Personality(5,
                                        "Basic",
                                        SlotDataCollection(p2_slot_data)));
    personalities.push_back(Personality(0, "No Channels"));
    personalities.push_back(Personality(3,  // One more slot than highest
                                        "Quirks Mode",
                                        SlotDataCollection(p4_slot_data)));
    instance = new Personalities(personalities);
  }
  return instance;
}

MovingLightResponder::Personalities *
  MovingLightResponder::Personalities::instance = NULL;

const ResponderOps<MovingLightResponder>::ParamHandler
    MovingLightResponder::PARAM_HANDLERS[] = {
  { PID_PARAMETER_DESCRIPTION,
    &MovingLightResponder::GetParamDescription,
    NULL},
  { PID_DEVICE_INFO,
    &MovingLightResponder::GetDeviceInfo,
    NULL},
  { PID_PRODUCT_DETAIL_ID_LIST,
    &MovingLightResponder::GetProductDetailList,
    NULL},
  { PID_DEVICE_MODEL_DESCRIPTION,
    &MovingLightResponder::GetDeviceModelDescription,
    NULL},
  { PID_MANUFACTURER_LABEL,
    &MovingLightResponder::GetManufacturerLabel,
    NULL},
  { PID_DEVICE_LABEL,
    &MovingLightResponder::GetDeviceLabel,
    &MovingLightResponder::SetDeviceLabel},
  { PID_FACTORY_DEFAULTS,
    &MovingLightResponder::GetFactoryDefaults,
    &MovingLightResponder::SetFactoryDefaults},
  { PID_LANGUAGE_CAPABILITIES,
    &MovingLightResponder::GetLanguageCapabilities,
    NULL},
  { PID_LANGUAGE,
    &MovingLightResponder::GetLanguage,
    &MovingLightResponder::SetLanguage},
  { PID_SOFTWARE_VERSION_LABEL,
    &MovingLightResponder::GetSoftwareVersionLabel,
    NULL},
  { PID_DMX_PERSONALITY,
    &MovingLightResponder::GetPersonality,
    &MovingLightResponder::SetPersonality},
  { PID_DMX_PERSONALITY_DESCRIPTION,
    &MovingLightResponder::GetPersonalityDescription,
    NULL},
  { PID_SLOT_INFO,
    &MovingLightResponder::GetSlotInfo,
    NULL},
  { PID_SLOT_DESCRIPTION,
    &MovingLightResponder::GetSlotDescription,
    NULL},
  { PID_DEFAULT_SLOT_VALUE,
    &MovingLightResponder::GetSlotDefaultValues,
    NULL},
  { PID_DMX_START_ADDRESS,
    &MovingLightResponder::GetDmxStartAddress,
    &MovingLightResponder::SetDmxStartAddress},
  { PID_DEVICE_HOURS,
    &MovingLightResponder::GetDeviceHours,
    &MovingLightResponder::SetDeviceHours},
  { PID_LAMP_HOURS,
    &MovingLightResponder::GetLampHours,
    &MovingLightResponder::SetLampHours},
  { PID_LAMP_STRIKES,
    &MovingLightResponder::GetLampStrikes,
    &MovingLightResponder::SetLampStrikes},
  { PID_LAMP_STATE,
    &MovingLightResponder::GetLampState,
    &MovingLightResponder::SetLampState},
  { PID_LAMP_ON_MODE,
    &MovingLightResponder::GetLampOnMode,
    &MovingLightResponder::SetLampOnMode},
  { PID_DEVICE_POWER_CYCLES,
    &MovingLightResponder::GetDevicePowerCycles,
    &MovingLightResponder::SetDevicePowerCycles},
  { PID_IDENTIFY_DEVICE,
    &MovingLightResponder::GetIdentify,
    &MovingLightResponder::SetIdentify},
  { PID_DISPLAY_INVERT,
    &MovingLightResponder::GetDisplayInvert,
    &MovingLightResponder::SetDisplayInvert},
  { PID_DISPLAY_LEVEL,
    &MovingLightResponder::GetDisplayLevel,
    &MovingLightResponder::SetDisplayLevel},
  { PID_PAN_INVERT,
    &MovingLightResponder::GetPanInvert,
    &MovingLightResponder::SetPanInvert},
  { PID_TILT_INVERT,
    &MovingLightResponder::GetTiltInvert,
    &MovingLightResponder::SetTiltInvert},
  { PID_PAN_TILT_SWAP,
    &MovingLightResponder::GetPanTiltSwap,
    &MovingLightResponder::SetPanTiltSwap},
  { PID_REAL_TIME_CLOCK,
    &MovingLightResponder::GetRealTimeClock,
    NULL},
  { PID_RESET_DEVICE,
    NULL,
    &MovingLightResponder::SetResetDevice},
  { PID_POWER_STATE,
    &MovingLightResponder::GetPowerState,
    &MovingLightResponder::SetPowerState},
  { OLA_MANUFACTURER_PID_CODE_VERSION,
    &MovingLightResponder::GetOlaCodeVersion,
    NULL},
  { 0, NULL, NULL},
};

/**
 * New MovingLightResponder
 */
MovingLightResponder::MovingLightResponder(const UID &uid)
    : m_uid(uid),
      m_start_address(1),
      m_language("en"),
      m_identify_mode(false),
      m_pan_invert(false),
      m_tilt_invert(false),
      m_device_hours(0),
      m_lamp_hours(0),
      m_lamp_strikes(0),
      m_lamp_state(LAMP_ON),
      m_lamp_on_mode(LAMP_ON_MODE_DMX),
      m_device_power_cycles(0),
      m_display_invert(DISPLAY_INVERT_AUTO),
      m_display_level(255),
      m_pan_tilt_swap(false),
      m_power_state(POWER_STATE_NORMAL),
      m_device_label("Dummy Moving Light"),
      m_personality_manager(Personalities::Instance()) {
}

/*
 * Handle an RDM Request
 */
void MovingLightResponder::SendRDMRequest(RDMRequest *request,
                                          RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}

RDMResponse *MovingLightResponder::GetParamDescription(
    const RDMRequest *request) {
  // Check that it's MANUFACTURER_PID_CODE_VERSION being requested
  uint16_t parameter_id;
  if (!ResponderHelper::ExtractUInt16(request, &parameter_id)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (parameter_id != OLA_MANUFACTURER_PID_CODE_VERSION) {
    OLA_WARN << "Dummy responder received param description request with "
      "unknown PID, expected " << OLA_MANUFACTURER_PID_CODE_VERSION
      << ", got " << parameter_id;
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else {
    return ResponderHelper::GetASCIIParamDescription(
        request,
        OLA_MANUFACTURER_PID_CODE_VERSION,
        CC_GET,
        "Code Version");
  }
}

RDMResponse *MovingLightResponder::GetDeviceInfo(
    const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_DUMMY_MOVING_LIGHT_MODEL,
      PRODUCT_CATEGORY_FIXTURE_MOVING_YOKE, 2,
      &m_personality_manager,
      m_start_address,
      0, 0);
}


/**
 * Reset to factory defaults
 */
RDMResponse *MovingLightResponder::GetFactoryDefaults(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint8_t using_defaults = (
      m_start_address == 1 &&
      m_personality_manager.ActivePersonalityNumber() == 1 &&
      m_identify_mode == false);
  return GetResponseFromData(request,
                             &using_defaults,
                             sizeof(using_defaults));
}

RDMResponse *MovingLightResponder::SetFactoryDefaults(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  m_start_address = 1;
  m_personality_manager.SetActivePersonality(1);
  m_identify_mode = 0;

  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *MovingLightResponder::GetLanguageCapabilities(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  const char languages[] = {
    'e', 'n',
    'f', 'r',
    'd', 'e',
  };

  return GetResponseFromData(request,
                             reinterpret_cast<const uint8_t*>(languages),
                             arraysize(languages));
}

RDMResponse *MovingLightResponder::GetLanguage(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  return GetResponseFromData(
      request,
      reinterpret_cast<const uint8_t*>(m_language.c_str()),
      m_language.size());
}

RDMResponse *MovingLightResponder::SetLanguage(
    const RDMRequest *request) {
  if (request->ParamDataSize() != 2) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  const string new_lang(reinterpret_cast<const char*>(request->ParamData()),
                        request->ParamDataSize());
  if (new_lang != "en" && new_lang != "fr" && new_lang != "de") {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }
  m_language = new_lang;

  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *MovingLightResponder::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(
      request, vector<rdm_product_detail>(1, PRODUCT_DETAIL_TEST));
}

RDMResponse *MovingLightResponder::GetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonality(request, &m_personality_manager);
}

RDMResponse *MovingLightResponder::SetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::SetPersonality(request, &m_personality_manager,
                                         m_start_address);
}

RDMResponse *MovingLightResponder::GetPersonalityDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonalityDescription(
      request, &m_personality_manager);
}

RDMResponse *MovingLightResponder::GetSlotInfo(
    const RDMRequest *request) {
  return ResponderHelper::GetSlotInfo(request, &m_personality_manager);
}

RDMResponse *MovingLightResponder::GetSlotDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetSlotDescription(request, &m_personality_manager);
}

RDMResponse *MovingLightResponder::GetSlotDefaultValues(
    const RDMRequest *request) {
  return ResponderHelper::GetSlotDefaultValues(request, &m_personality_manager);
}

RDMResponse *MovingLightResponder::GetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::GetDmxAddress(request, &m_personality_manager,
                                        m_start_address);
}

RDMResponse *MovingLightResponder::SetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::SetDmxAddress(request, &m_personality_manager,
                                        &m_start_address);
}

RDMResponse *MovingLightResponder::GetDeviceHours(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt32Value(request, m_device_hours++);
}

RDMResponse *MovingLightResponder::SetDeviceHours(
    const RDMRequest *request) {
  return ResponderHelper::SetUInt32Value(request, &m_device_hours);
}

RDMResponse *MovingLightResponder::GetLampHours(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt32Value(request, m_lamp_hours++);
}

RDMResponse *MovingLightResponder::SetLampHours(
    const RDMRequest *request) {
  return ResponderHelper::SetUInt32Value(request, &m_lamp_hours);
}

RDMResponse *MovingLightResponder::GetLampStrikes(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt32Value(request, m_lamp_strikes);
}

RDMResponse *MovingLightResponder::SetLampStrikes(
    const RDMRequest *request) {
  return ResponderHelper::SetUInt32Value(request, &m_lamp_strikes);
}

RDMResponse *MovingLightResponder::GetLampState(
    const RDMRequest *request) {
  uint8_t value = m_lamp_state;
  return ResponderHelper::GetUInt8Value(request, value);
}

RDMResponse *MovingLightResponder::SetLampState(
    const RDMRequest *request) {
  uint8_t new_value;
  if (!ResponderHelper::ExtractUInt8(request, &new_value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (new_value > static_cast<uint8_t>(LAMP_STANDBY)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_lamp_state = static_cast<rdm_lamp_state>(new_value);
  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *MovingLightResponder::GetLampOnMode(
    const RDMRequest *request) {
  uint8_t value = m_lamp_on_mode;
  return ResponderHelper::GetUInt8Value(request, value);
}

RDMResponse *MovingLightResponder::SetLampOnMode(
    const RDMRequest *request) {
  uint8_t new_value;
  if (!ResponderHelper::ExtractUInt8(request, &new_value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (new_value > static_cast<uint8_t>(LAMP_ON_MODE_ON_AFTER_CAL)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_lamp_on_mode = static_cast<rdm_lamp_mode>(new_value);
  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *MovingLightResponder::GetDevicePowerCycles(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt32Value(request, m_device_power_cycles++);
}

RDMResponse *MovingLightResponder::SetDevicePowerCycles(
    const RDMRequest *request) {
  return ResponderHelper::SetUInt32Value(request, &m_device_power_cycles);
}

RDMResponse *MovingLightResponder::GetIdentify(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

RDMResponse *MovingLightResponder::SetIdentify(
    const RDMRequest *request) {
  bool old_value = m_identify_mode;
  RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "Dummy Moving Light " << m_uid << ", identify mode "
             << (m_identify_mode ? "on" : "off");
  }
  return response;
}

RDMResponse *MovingLightResponder::GetDisplayInvert(
    const RDMRequest *request) {
  uint8_t value = m_display_invert;
  return ResponderHelper::GetUInt8Value(request, value);
}

RDMResponse *MovingLightResponder::SetDisplayInvert(
    const RDMRequest *request) {
  uint8_t new_value;
  if (!ResponderHelper::ExtractUInt8(request, &new_value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (new_value > static_cast<uint8_t>(DISPLAY_INVERT_AUTO)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_display_invert = static_cast<rdm_display_invert>(new_value);
  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *MovingLightResponder::GetDisplayLevel(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt8Value(request, m_display_level);
}

RDMResponse *MovingLightResponder::SetDisplayLevel(
    const RDMRequest *request) {
  return ResponderHelper::SetUInt8Value(request, &m_display_level);
}

RDMResponse *MovingLightResponder::GetPanInvert(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_pan_invert);
}

RDMResponse *MovingLightResponder::SetPanInvert(
    const RDMRequest *request) {
  return ResponderHelper::SetBoolValue(request, &m_pan_invert);
}

RDMResponse *MovingLightResponder::GetTiltInvert(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_tilt_invert);
}

RDMResponse *MovingLightResponder::SetTiltInvert(
    const RDMRequest *request) {
  return ResponderHelper::SetBoolValue(request, &m_tilt_invert);
}

RDMResponse *MovingLightResponder::GetPanTiltSwap(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_pan_tilt_swap);
}

RDMResponse *MovingLightResponder::SetPanTiltSwap(
    const RDMRequest *request) {
  return ResponderHelper::SetBoolValue(request, &m_pan_tilt_swap);
}

RDMResponse *MovingLightResponder::GetRealTimeClock(
    const RDMRequest *request) {
  return ResponderHelper::GetRealTimeClock(request);
}

RDMResponse *MovingLightResponder::GetPowerState(
    const RDMRequest *request) {
  uint8_t value = m_power_state;
  return ResponderHelper::GetUInt8Value(request, value);
}

RDMResponse *MovingLightResponder::SetPowerState(
    const RDMRequest *request) {
  uint8_t new_value;
  if (!ResponderHelper::ExtractUInt8(request, &new_value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (!UIntToPowerState(new_value, &m_power_state)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }
  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *MovingLightResponder::SetResetDevice(
    const RDMRequest *request) {
  uint8_t value;
  rdm_reset_device_mode reset_device_enum;
  if (!ResponderHelper::ExtractUInt8(request, &value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (!UIntToResetDevice(value, &reset_device_enum)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  string reset_type = ResetDeviceToString(reset_device_enum);
  ToLower(&reset_type);

  OLA_INFO << "Dummy Moving Light " << m_uid << " " << reset_type <<
      " reset device";

  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *MovingLightResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA Moving Light");
}

RDMResponse *MovingLightResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

RDMResponse *MovingLightResponder::GetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, m_device_label);
}

RDMResponse *MovingLightResponder::SetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::SetString(request, &m_device_label);
}

RDMResponse *MovingLightResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

RDMResponse *MovingLightResponder::GetOlaCodeVersion(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, VERSION);
}
}  // namespace rdm
}  // namespace ola
