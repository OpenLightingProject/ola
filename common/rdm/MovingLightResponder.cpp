/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * MovingLightResponder.cpp
 * Copyright (C) 2013 Simon Newton
 */

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

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

MovingLightResponder::RDMOps *MovingLightResponder::RDMOps::instance = NULL;

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
    NULL},
  { PID_FACTORY_DEFAULTS,
    &MovingLightResponder::GetFactoryDefaults,
    &MovingLightResponder::SetFactoryDefaults},
  { PID_SOFTWARE_VERSION_LABEL,
    &MovingLightResponder::GetSoftwareVersionLabel,
    NULL},
  { PID_DMX_PERSONALITY,
    &MovingLightResponder::GetPersonality,
    &MovingLightResponder::SetPersonality},
  { PID_DMX_PERSONALITY_DESCRIPTION,
    &MovingLightResponder::GetPersonalityDescription,
    NULL},
  { PID_DMX_START_ADDRESS,
    &MovingLightResponder::GetDmxStartAddress,
    &MovingLightResponder::SetDmxStartAddress},
  { PID_LAMP_STRIKES,
    &MovingLightResponder::GetLampStrikes,
    &MovingLightResponder::SetLampStrikes},
  { PID_IDENTIFY_DEVICE,
    &MovingLightResponder::GetIdentify,
    &MovingLightResponder::SetIdentify},
  { PID_PAN_INVERT,
    &MovingLightResponder::GetPanInvert,
    &MovingLightResponder::SetPanInvert},
  { PID_TILT_INVERT,
    &MovingLightResponder::GetTiltInvert,
    &MovingLightResponder::SetTiltInvert},
  { PID_REAL_TIME_CLOCK,
    &MovingLightResponder::GetRealTimeClock,
    NULL},
  { OLA_MANUFACTURER_PID_CODE_VERSION,
    &MovingLightResponder::GetOlaCodeVersion,
    NULL},
  { 0, NULL, NULL},
};

const MovingLightResponder::personality_info
    MovingLightResponder::PERSONALITIES[] = {
  {0, "Personality 1"},
  {5, "Personality 2"},
  {10, "Personality 3"},
  {20, "Personality 4"},
};


/**
 * New MovingLightResponder
 */
MovingLightResponder::MovingLightResponder(const UID &uid)
    : m_uid(uid),
      m_start_address(1),
      m_personality(1),
      m_identify_mode(0),
      m_pan_invert(0),
      m_tilt_invert(0),
      m_lamp_strikes(0) {
}

/*
 * Handle an RDM Request
 */
void MovingLightResponder::SendRDMRequest(const RDMRequest *request,
                                    RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE,
                                       request, callback);
}

RDMResponse *MovingLightResponder::GetParamDescription(
    const RDMRequest *request) {
  // Check that it's MANUFACTURER_PID_CODE_VERSION being requested
  uint16_t parameter_id;
  if (request->ParamDataSize() != sizeof(parameter_id)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&parameter_id),
         request->ParamData(),
         sizeof(parameter_id));
  parameter_id = NetworkToHost(parameter_id);

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

RDMResponse *MovingLightResponder::GetDeviceInfo(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  struct device_info_s {
    uint16_t rdm_version;
    uint16_t model;
    uint16_t product_category;
    uint32_t software_version;
    uint16_t dmx_footprint;
    uint8_t current_personality;
    uint8_t personality_count;
    uint16_t dmx_start_address;
    uint16_t sub_device_count;
    uint8_t sensor_count;
  } __attribute__((packed));

  struct device_info_s device_info;
  device_info.rdm_version = HostToNetwork(static_cast<uint16_t>(0x100));
  device_info.model = HostToNetwork(
      static_cast<uint16_t>(OLA_DUMMY_MOVING_LIGHT_MODEL));
  device_info.product_category = HostToNetwork(
      static_cast<uint16_t>(PRODUCT_CATEGORY_FIXTURE_MOVING_YOKE));
  device_info.software_version = HostToNetwork(static_cast<uint32_t>(1));
  device_info.dmx_footprint = HostToNetwork(Footprint());
  device_info.current_personality = m_personality + 1;
  device_info.personality_count = arraysize(PERSONALITIES);
  device_info.dmx_start_address = device_info.dmx_footprint ?
    HostToNetwork(m_start_address) : 0xffff;
  device_info.sub_device_count = 0;
  device_info.sensor_count = 0;
  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&device_info),
      sizeof(device_info));
}


/**
 * Reset to factory defaults
 */
RDMResponse *MovingLightResponder::GetFactoryDefaults(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint8_t using_defaults = (m_start_address == 1 && m_personality == 1 &&
                            m_identify_mode == false);
  return GetResponseFromData(
    request,
    &using_defaults,
    sizeof(using_defaults));
}


RDMResponse *MovingLightResponder::SetFactoryDefaults(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  m_start_address = 1;
  m_personality = 1;
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

RDMResponse *MovingLightResponder::GetProductDetailList(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint16_t product_details[] = {
    PRODUCT_DETAIL_TEST,
  };

  for (unsigned int i = 0; i < arraysize(product_details); i++)
    product_details[i] = HostToNetwork(product_details[i]);

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&product_details),
      sizeof(product_details));
}

RDMResponse *MovingLightResponder::GetPersonality(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  struct personality_info_s {
    uint8_t personality;
    uint8_t total;
  } __attribute__((packed));

  struct personality_info_s personality_info;
  personality_info.personality = m_personality + 1;
  personality_info.total = arraysize(PERSONALITIES);
  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&personality_info),
    sizeof(personality_info));
}

RDMResponse *MovingLightResponder::SetPersonality(
    const RDMRequest *request) {
  uint8_t personality;
  if (request->ParamDataSize() != sizeof(personality)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&personality), request->ParamData(),
         sizeof(personality));
  personality = NetworkToHost(personality);

  if (personality > arraysize(PERSONALITIES) || personality == 0) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else if (m_start_address + PERSONALITIES[personality - 1].footprint - 1
             > DMX_UNIVERSE_SIZE) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else {
    m_personality = personality - 1;
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
}


RDMResponse *MovingLightResponder::GetPersonalityDescription(
    const RDMRequest *request) {
  uint8_t personality = 0;
  if (request->ParamDataSize() != sizeof(personality)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&personality), request->ParamData(),
         sizeof(personality));
  personality = NetworkToHost(personality) - 1;
  if (personality >= arraysize(PERSONALITIES)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else {
    struct personality_description_s {
      uint8_t personality;
      uint16_t slots_required;
      char description[MAX_RDM_STRING_LENGTH];
    } __attribute__((packed));

    struct personality_description_s personality_description;
    personality_description.personality = personality + 1;
    personality_description.slots_required =
        HostToNetwork(PERSONALITIES[personality].footprint);
    strncpy(personality_description.description,
            PERSONALITIES[personality].description,
            sizeof(personality_description.description));

    return GetResponseFromData(
        request,
        reinterpret_cast<uint8_t*>(&personality_description),
        sizeof(personality_description));
  }
}

RDMResponse *MovingLightResponder::GetDmxStartAddress(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint16_t address = HostToNetwork(m_start_address);
  if (Footprint() == 0)
    address = 0xffff;
  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&address),
    sizeof(address));
}

RDMResponse *MovingLightResponder::SetDmxStartAddress(
    const RDMRequest *request) {
  uint16_t address;
  if (request->ParamDataSize() != sizeof(address)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&address), request->ParamData(),
         sizeof(address));
  address = NetworkToHost(address);
  uint16_t end_address = DMX_UNIVERSE_SIZE - Footprint() + 1;
  if (address == 0 || address > end_address) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else if (Footprint() == 0) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else {
    m_start_address = address;
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
}

RDMResponse *MovingLightResponder::GetLampStrikes(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint32_t strikes = HostToNetwork(m_lamp_strikes);
  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&strikes),
    sizeof(strikes));
}

RDMResponse *MovingLightResponder::SetLampStrikes(
    const RDMRequest *request) {
  uint32_t lamp_strikes;
  if (request->ParamDataSize() != sizeof(lamp_strikes)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&lamp_strikes), request->ParamData(),
         sizeof(lamp_strikes));
  m_lamp_strikes = HostToNetwork(lamp_strikes);
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

RDMResponse *MovingLightResponder::GetIdentify(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  return GetResponseFromData(
      request,
      &m_identify_mode,
      sizeof(m_identify_mode));
}

RDMResponse *MovingLightResponder::SetIdentify(
    const RDMRequest *request) {
  uint8_t mode;
  if (request->ParamDataSize() != sizeof(mode)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  mode = *request->ParamData();
  if (mode == 0 || mode == 1) {
    m_identify_mode = mode;
    OLA_INFO << "Dummy Moving Light " << m_uid << ", identify mode "
             << (m_identify_mode ? "on" : "off");
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
  } else {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }
}

RDMResponse *MovingLightResponder::GetPanInvert(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }
  return GetResponseFromData(request, &m_pan_invert, sizeof(m_pan_invert));
}


RDMResponse *MovingLightResponder::SetPanInvert(
    const RDMRequest *request) {
  return SetBoolValue(request, &m_pan_invert);
}

RDMResponse *MovingLightResponder::GetTiltInvert(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }
  return GetResponseFromData(request, &m_tilt_invert, sizeof(m_tilt_invert));
}

RDMResponse *MovingLightResponder::SetTiltInvert(
    const RDMRequest *request) {
  return SetBoolValue(request, &m_tilt_invert);
}

RDMResponse *MovingLightResponder::GetRealTimeClock(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  struct clock_s {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  } __attribute__((packed));

  time_t now;
  now = time(NULL);
  struct tm tm_now;
  localtime_r(&now, &tm_now);

  struct clock_s clock;
  clock.year = HostToNetwork(static_cast<uint16_t>(1900 + tm_now.tm_year));
  clock.month = tm_now.tm_mon + 1;
  clock.day = tm_now.tm_mday;
  clock.hour = tm_now.tm_hour;
  clock.minute = tm_now.tm_min;
  clock.second = tm_now.tm_sec;

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&clock),
      sizeof(clock));
}

RDMResponse *MovingLightResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return HandleStringResponse(request, "OLA Moving Light");
}

RDMResponse *MovingLightResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return HandleStringResponse(request, "Open Lighting Project");
}

RDMResponse *MovingLightResponder::GetDeviceLabel(
    const RDMRequest *request) {
  return HandleStringResponse(request, "Dummy Moving Light");
}

RDMResponse *MovingLightResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return HandleStringResponse(request, string("OLA Version ") + VERSION);
}

RDMResponse *MovingLightResponder::GetOlaCodeVersion(
    const RDMRequest *request) {
  return HandleStringResponse(request, VERSION);
}

/*
 * Handle a request that returns a string
 */
RDMResponse *MovingLightResponder::HandleStringResponse(
    const RDMRequest *request,
    const std::string &value) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }
  return GetResponseFromData(
        request,
        reinterpret_cast<const uint8_t*>(value.data()),
        value.size());
}

RDMResponse *MovingLightResponder::SetBoolValue(
    const RDMRequest *request,
    uint8_t *value) {
  uint8_t arg;
  if (request->ParamDataSize() != sizeof(arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  arg = *request->ParamData();
  if (arg == 0 || arg == 1) {
    *value = arg;
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
  } else {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }
}
}  // namespace rdm
}  // namespace ola
