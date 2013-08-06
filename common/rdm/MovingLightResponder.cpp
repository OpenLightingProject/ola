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
 * MovingLightResponder.cpp
 * Copyright (C) 2013 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/MovingLightResponder.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/ResponderSlotData.h"

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
    SlotDataCollection::SlotDataList p2_slot_data;
    p2_slot_data.push_back(new SlotData(ST_PRIMARY, SD_INTENSITY, 0, "Int"));
    p2_slot_data.push_back(new SlotData(ST_SEC_FINE, SD_INTENSITY, 0));
    p2_slot_data.push_back(new SlotData(ST_PRIMARY, SD_PAN, 127));
    //SlotDatas p2_sdc = new SlotDatas(p2_slot_data);
    PersonalityList personalities;
    personalities.push_back(new Personality(0, "Personality 1"));
    //personalities.push_back(new Personality(5, "Personality 2", p2_sdc));
    personalities.push_back(new Personality(5, "Personality 2", SlotDataCollection(p2_slot_data)));
    //personalities.push_back(new Personality(5, "Personality 2"));
    personalities.push_back(new Personality(10, "Personality 3"));
    personalities.push_back(new Personality(20, "Personality 4"));
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
void MovingLightResponder::SendRDMRequest(const RDMRequest *request,
                                          RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}

const RDMResponse *MovingLightResponder::GetParamDescription(
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
    struct parameter_description_s {
      uint16_t pid;
      uint8_t pdl_size;
      uint8_t data_type;
      uint8_t command_class;
      uint8_t type;
      uint8_t unit;
      uint8_t prefix;
      uint32_t min_value;
      uint32_t default_value;
      uint32_t max_value;
      char description[MAX_RDM_STRING_LENGTH];
    } __attribute__((packed));

    struct parameter_description_s param_description;
    param_description.pid = HostToNetwork(
        static_cast<uint16_t>(OLA_MANUFACTURER_PID_CODE_VERSION));
    param_description.pdl_size = HostToNetwork(
        static_cast<uint8_t>(MAX_RDM_STRING_LENGTH));
    param_description.data_type = HostToNetwork(
        static_cast<uint8_t>(DS_ASCII));
    param_description.command_class = HostToNetwork(
        static_cast<uint8_t>(CC_GET));
    param_description.type = 0;
    param_description.unit = HostToNetwork(
        static_cast<uint8_t>(UNITS_NONE));
    param_description.prefix = HostToNetwork(
        static_cast<uint8_t>(PREFIX_NONE));
    param_description.min_value = 0;
    param_description.default_value = 0;
    param_description.max_value = 0;
    strncpy(param_description.description, "Code Version",
            MAX_RDM_STRING_LENGTH);
    return GetResponseFromData(
        request,
        reinterpret_cast<uint8_t*>(&param_description),
        sizeof(param_description));
  }
}

const RDMResponse *MovingLightResponder::GetDeviceInfo(
    const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_DUMMY_MOVING_LIGHT_MODEL,
      PRODUCT_CATEGORY_FIXTURE_MOVING_YOKE, 1,
      &m_personality_manager,
      m_start_address,
      0, 0);
}


/**
 * Reset to factory defaults
 */
const RDMResponse *MovingLightResponder::GetFactoryDefaults(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint8_t using_defaults = (
      m_start_address == 1 &&
      m_personality_manager.ActivePersonalityNumber() == 1 &&
      m_identify_mode == false);
  return GetResponseFromData(
    request,
    &using_defaults,
    sizeof(using_defaults));
}

const RDMResponse *MovingLightResponder::SetFactoryDefaults(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  m_start_address = 1;
  m_personality_manager.SetActivePersonality(1);
  m_identify_mode = 0;

  return new RDMSetResponse(
    request->DestinationUID(),
    request->SourceUID(),
    request->TransactionNumber(),
    RDM_ACK,
    0,
    request->SubDevice(),
    request->ParamId(),
    NULL,
    0);
}

const RDMResponse *MovingLightResponder::GetLanguageCapabilities(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  const char languages[] = {
    'e', 'n',
    'f', 'r',
    'd', 'e',
  };
  return new RDMGetResponse(
    request->DestinationUID(),
    request->SourceUID(),
    request->TransactionNumber(),
    RDM_ACK,
    0,
    request->SubDevice(),
    request->ParamId(),
    reinterpret_cast<const uint8_t*>(languages),
    arraysize(languages));
}

const RDMResponse *MovingLightResponder::GetLanguage(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  return new RDMGetResponse(
    request->DestinationUID(),
    request->SourceUID(),
    request->TransactionNumber(),
    RDM_ACK,
    0,
    request->SubDevice(),
    request->ParamId(),
    reinterpret_cast<const uint8_t*>(m_language.c_str()),
    m_language.size());
}

const RDMResponse *MovingLightResponder::SetLanguage(
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

  return new RDMSetResponse(
    request->DestinationUID(),
    request->SourceUID(),
    request->TransactionNumber(),
    RDM_ACK,
    0,
    request->SubDevice(),
    request->ParamId(),
    NULL,
    0);
}

const RDMResponse *MovingLightResponder::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(
      request, vector<rdm_product_detail>(1, PRODUCT_DETAIL_TEST));
}

const RDMResponse *MovingLightResponder::GetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonality(request, &m_personality_manager);
}

const RDMResponse *MovingLightResponder::SetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::SetPersonality(request, &m_personality_manager,
                                         m_start_address);
}

const RDMResponse *MovingLightResponder::GetPersonalityDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonalityDescription(
      request, &m_personality_manager);
}

const RDMResponse *MovingLightResponder::GetSlotInfo(
    const RDMRequest *request) {
  return ResponderHelper::GetSlotInfo(
      request, &m_personality_manager);
}

const RDMResponse *MovingLightResponder::GetSlotDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetSlotDescription(
      request, &m_personality_manager);
}

const RDMResponse *MovingLightResponder::GetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::GetDmxAddress(request, &m_personality_manager,
                                        m_start_address);
}

const RDMResponse *MovingLightResponder::SetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::SetDmxAddress(request, &m_personality_manager,
                                        &m_start_address);
}

const RDMResponse *MovingLightResponder::GetDeviceHours(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt32Value(request, m_device_hours++);
}

const RDMResponse *MovingLightResponder::SetDeviceHours(
    const RDMRequest *request) {
  return ResponderHelper::SetUInt32Value(request, &m_device_hours);
}

const RDMResponse *MovingLightResponder::GetLampHours(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt32Value(request, m_lamp_hours++);
}

const RDMResponse *MovingLightResponder::SetLampHours(
    const RDMRequest *request) {
  return ResponderHelper::SetUInt32Value(request, &m_lamp_hours);
}

const RDMResponse *MovingLightResponder::GetLampStrikes(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt32Value(request, m_lamp_strikes);
}

const RDMResponse *MovingLightResponder::SetLampStrikes(
    const RDMRequest *request) {
  return ResponderHelper::SetUInt32Value(request, &m_lamp_strikes);
}

const RDMResponse *MovingLightResponder::GetLampState(
    const RDMRequest *request) {
  uint8_t value = m_lamp_state;
  return ResponderHelper::GetUInt8Value(request, value);
}

const RDMResponse *MovingLightResponder::SetLampState(
    const RDMRequest *request) {
  uint8_t new_value;
  if (!ResponderHelper::ExtractUInt8(request, &new_value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (new_value > static_cast<uint8_t>(LAMP_STANDBY)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_lamp_state = static_cast<rdm_lamp_state>(new_value);
  return new RDMSetResponse(
    request->DestinationUID(),
    request->SourceUID(),
    request->TransactionNumber(),
    RDM_ACK,
    0,
    request->SubDevice(),
    request->ParamId(),
    NULL,
    0);
}

const RDMResponse *MovingLightResponder::GetLampOnMode(
    const RDMRequest *request) {
  uint8_t value = m_lamp_on_mode;
  return ResponderHelper::GetUInt8Value(request, value);
}

const RDMResponse *MovingLightResponder::SetLampOnMode(
    const RDMRequest *request) {
  uint8_t new_value;
  if (!ResponderHelper::ExtractUInt8(request, &new_value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (new_value > static_cast<uint8_t>(LAMP_ON_MODE_AFTER_CAL)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_lamp_on_mode = static_cast<rdm_lamp_mode>(new_value);
  return new RDMSetResponse(
    request->DestinationUID(),
    request->SourceUID(),
    request->TransactionNumber(),
    RDM_ACK,
    0,
    request->SubDevice(),
    request->ParamId(),
    NULL,
    0);
}

const RDMResponse *MovingLightResponder::GetDevicePowerCycles(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt32Value(request, m_device_power_cycles++);
}

const RDMResponse *MovingLightResponder::SetDevicePowerCycles(
    const RDMRequest *request) {
  return ResponderHelper::SetUInt32Value(request, &m_device_power_cycles);
}

const RDMResponse *MovingLightResponder::GetIdentify(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

const RDMResponse *MovingLightResponder::SetIdentify(
    const RDMRequest *request) {
  bool old_value = m_identify_mode;
  const RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "Dummy Moving Light " << m_uid << ", identify mode "
             << (m_identify_mode ? "on" : "off");
  }
  return response;
}

const RDMResponse *MovingLightResponder::GetDisplayInvert(
    const RDMRequest *request) {
  uint8_t value = m_display_invert;
  return ResponderHelper::GetUInt8Value(request, value);
}

const RDMResponse *MovingLightResponder::SetDisplayInvert(
    const RDMRequest *request) {
  uint8_t new_value;
  if (!ResponderHelper::ExtractUInt8(request, &new_value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (new_value > static_cast<uint8_t>(DISPLAY_INVERT_AUTO)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_display_invert = static_cast<rdm_display_invert>(new_value);
  return new RDMSetResponse(
    request->DestinationUID(),
    request->SourceUID(),
    request->TransactionNumber(),
    RDM_ACK,
    0,
    request->SubDevice(),
    request->ParamId(),
    NULL,
    0);
}

const RDMResponse *MovingLightResponder::GetDisplayLevel(
    const RDMRequest *request) {
  return ResponderHelper::GetUInt8Value(request, m_display_level);
}

const RDMResponse *MovingLightResponder::SetDisplayLevel(
    const RDMRequest *request) {
  return ResponderHelper::SetUInt8Value(request, &m_display_level);
}

const RDMResponse *MovingLightResponder::GetPanInvert(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_pan_invert);
}

const RDMResponse *MovingLightResponder::SetPanInvert(
    const RDMRequest *request) {
  return ResponderHelper::SetBoolValue(request, &m_pan_invert);
}

const RDMResponse *MovingLightResponder::GetTiltInvert(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_tilt_invert);
}

const RDMResponse *MovingLightResponder::SetTiltInvert(
    const RDMRequest *request) {
  return ResponderHelper::SetBoolValue(request, &m_tilt_invert);
}

const RDMResponse *MovingLightResponder::GetPanTiltSwap(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_pan_tilt_swap);
}

const RDMResponse *MovingLightResponder::SetPanTiltSwap(
    const RDMRequest *request) {
  return ResponderHelper::SetBoolValue(request, &m_pan_tilt_swap);
}

const RDMResponse *MovingLightResponder::GetRealTimeClock(
    const RDMRequest *request) {
  return ResponderHelper::GetRealTimeClock(request);
}

const RDMResponse *MovingLightResponder::GetPowerState(
    const RDMRequest *request) {
  uint8_t value = m_power_state;
  return ResponderHelper::GetUInt8Value(request, value);
}

const RDMResponse *MovingLightResponder::SetPowerState(
    const RDMRequest *request) {
  uint8_t new_value;
  if (!ResponderHelper::ExtractUInt8(request, &new_value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (new_value > static_cast<uint8_t>(POWER_STATE_STANDBY) &&
      new_value != static_cast<uint8_t>(POWER_STATE_NORMAL)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  m_power_state = static_cast<rdm_power_state>(new_value);
  return ResponderHelper::EmptySetResponse(request);
}

const RDMResponse *MovingLightResponder::SetResetDevice(
    const RDMRequest *request) {
  uint8_t value;
  if (!ResponderHelper::ExtractUInt8(request, &value)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (value != static_cast<uint8_t>(RESET_WARM) &&
      value != static_cast<uint8_t>(RESET_COLD)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  OLA_INFO << "Dummy Moving Light " << m_uid << " " <<
      ((value == static_cast<uint8_t>(RESET_WARM)) ? "warm" : "cold") <<
      " reset device";

  return ResponderHelper::EmptySetResponse(request);
}

const RDMResponse *MovingLightResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA Moving Light");
}

const RDMResponse *MovingLightResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

const RDMResponse *MovingLightResponder::GetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, m_device_label);
}

const RDMResponse *MovingLightResponder::SetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::SetString(request, &m_device_label);
}

const RDMResponse *MovingLightResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

const RDMResponse *MovingLightResponder::GetOlaCodeVersion(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, VERSION);
}
}  // namespace rdm
}  // namespace ola
