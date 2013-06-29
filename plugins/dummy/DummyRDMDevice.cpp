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
 * DummyRDMDevice.cpp
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <iostream>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "plugins/dummy/DummyRDMDevice.h"

namespace ola {
namespace plugin {
namespace dummy {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using ola::rdm::GetResponseFromData;
using ola::rdm::NackWithReason;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::rdm_response_code;
using std::string;
using std::vector;

DummyRDMDevice::RDMOps *DummyRDMDevice::RDMOps::instance = NULL;

const ola::rdm::ResponderOps<DummyRDMDevice>::ParamHandler
    DummyRDMDevice::PARAM_HANDLERS[] = {
  { ola::rdm::PID_PARAMETER_DESCRIPTION,
    &DummyRDMDevice::GetParamDescription,
    NULL},
  { ola::rdm::PID_DEVICE_INFO,
    &DummyRDMDevice::GetDeviceInfo,
    NULL},
  { ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
    &DummyRDMDevice::GetProductDetailList,
    NULL},
  { ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
    &DummyRDMDevice::GetDeviceModelDescription,
    NULL},
  { ola::rdm::PID_MANUFACTURER_LABEL,
    &DummyRDMDevice::GetManufacturerLabel,
    NULL},
  { ola::rdm::PID_DEVICE_LABEL,
    &DummyRDMDevice::GetDeviceLabel,
    NULL},
  { ola::rdm::PID_FACTORY_DEFAULTS,
    &DummyRDMDevice::GetFactoryDefaults,
    &DummyRDMDevice::SetFactoryDefaults},
  { ola::rdm::PID_SOFTWARE_VERSION_LABEL,
    &DummyRDMDevice::GetSoftwareVersionLabel,
    NULL},
  { ola::rdm::PID_DMX_PERSONALITY,
    &DummyRDMDevice::GetPersonality,
    &DummyRDMDevice::SetPersonality},
  { ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION,
    &DummyRDMDevice::GetPersonalityDescription,
    NULL},
  { ola::rdm::PID_DMX_START_ADDRESS,
    &DummyRDMDevice::GetDmxStartAddress,
    &DummyRDMDevice::SetDmxStartAddress},
  { ola::rdm::PID_LAMP_STRIKES,
    &DummyRDMDevice::GetLampStrikes,
    &DummyRDMDevice::SetLampStrikes},
  { ola::rdm::PID_IDENTIFY_DEVICE,
    &DummyRDMDevice::GetIdentify,
    &DummyRDMDevice::SetIdentify},
  { ola::rdm::PID_REAL_TIME_CLOCK,
    &DummyRDMDevice::GetRealTimeClock,
    NULL},
  { ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION,
    &DummyRDMDevice::GetOlaCodeVersion,
    NULL},
  { 0, NULL, NULL},
};

const DummyRDMDevice::personality_info DummyRDMDevice::PERSONALITIES[] = {
  {0, "Personality 1"},
  {5, "Personality 2"},
  {10, "Personality 3"},
  {20, "Personality 4"},
};

/*
 * Handle an RDM Request
 */
void DummyRDMDevice::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                    ola::rdm::RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, m_sub_device_number,
                                       request, callback);
}

RDMResponse *DummyRDMDevice::GetParamDescription(
    const ola::rdm::RDMRequest *request) {
  // Check that it's MANUFACTURER_PID_CODE_VERSION being requested
  uint16_t parameter_id;
  if (request->ParamDataSize() != sizeof(parameter_id)) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&parameter_id),
         request->ParamData(),
         sizeof(parameter_id));
  parameter_id = NetworkToHost(parameter_id);

  if (parameter_id != ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION) {
    OLA_WARN << "Dummy responder received param description request with "
      "unknown PID, expected " << ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION
      << ", got " << parameter_id;
    return NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
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
      char description[ola::rdm::MAX_RDM_STRING_LENGTH];
    } __attribute__((packed));

    struct parameter_description_s param_description;
    param_description.pid = HostToNetwork(
        static_cast<uint16_t>(ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION));
    param_description.pdl_size = HostToNetwork(
        static_cast<uint8_t>(ola::rdm::MAX_RDM_STRING_LENGTH));
    param_description.data_type = HostToNetwork(
        static_cast<uint8_t>(ola::rdm::DS_ASCII));
    param_description.command_class = HostToNetwork(
        static_cast<uint8_t>(ola::rdm::CC_GET));
    param_description.type = 0;
    param_description.unit = HostToNetwork(
        static_cast<uint8_t>(ola::rdm::UNITS_NONE));
    param_description.prefix = HostToNetwork(
        static_cast<uint8_t>(ola::rdm::PREFIX_NONE));
    param_description.min_value = 0;
    param_description.default_value = 0;
    param_description.max_value = 0;
    strncpy(param_description.description, "Code Version",
            ola::rdm::MAX_RDM_STRING_LENGTH);
    return GetResponseFromData(
        request,
        reinterpret_cast<uint8_t*>(&param_description),
        sizeof(param_description));
  }
}

RDMResponse *DummyRDMDevice::GetDeviceInfo(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
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
  device_info.model = HostToNetwork(static_cast<uint16_t>(1));
  device_info.product_category = HostToNetwork(
      static_cast<uint16_t>(ola::rdm::PRODUCT_CATEGORY_OTHER));
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
RDMResponse *DummyRDMDevice::GetFactoryDefaults(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  uint8_t using_defaults = (m_start_address == 1 && m_personality == 1 &&
                            m_identify_mode == false);
  return GetResponseFromData(
    request,
    &using_defaults,
    sizeof(using_defaults));
}


RDMResponse *DummyRDMDevice::SetFactoryDefaults(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  m_start_address = 1;
  m_personality = 1;
  m_identify_mode = 0;

  return new ola::rdm::RDMSetResponse(
    request->DestinationUID(),
    request->SourceUID(),
    request->TransactionNumber(),
    ola::rdm::RDM_ACK,
    0,
    request->SubDevice(),
    request->ParamId(),
    NULL,
    0);
}

RDMResponse *DummyRDMDevice::GetProductDetailList(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  uint16_t product_details[] = {
    ola::rdm::PRODUCT_DETAIL_TEST,
    ola::rdm::PRODUCT_DETAIL_OTHER
  };

  for (unsigned int i = 0; i < arraysize(product_details); i++)
    product_details[i] = HostToNetwork(product_details[i]);

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&product_details),
      sizeof(product_details));
}

RDMResponse *DummyRDMDevice::GetPersonality(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
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

RDMResponse *DummyRDMDevice::SetPersonality(
    const ola::rdm::RDMRequest *request) {
  uint8_t personality;
  if (request->ParamDataSize() != sizeof(personality)) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&personality), request->ParamData(),
         sizeof(personality));
  personality = NetworkToHost(personality);

  if (personality > arraysize(PERSONALITIES) || personality == 0) {
    return NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  } else if (m_start_address + PERSONALITIES[personality - 1].footprint - 1
             > DMX_UNIVERSE_SIZE) {
    return NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  } else {
    m_personality = personality - 1;
    return new ola::rdm::RDMSetResponse(
      request->DestinationUID(),
      request->SourceUID(),
      request->TransactionNumber(),
      ola::rdm::RDM_ACK,
      0,
      request->SubDevice(),
      request->ParamId(),
      NULL,
      0);
  }
}


RDMResponse *DummyRDMDevice::GetPersonalityDescription(
    const ola::rdm::RDMRequest *request) {
  uint8_t personality = 0;
  if (request->ParamDataSize() != sizeof(personality)) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&personality), request->ParamData(),
         sizeof(personality));
  personality = NetworkToHost(personality) - 1;
  if (personality >= arraysize(PERSONALITIES)) {
    return NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  } else {
    struct personality_description_s {
      uint8_t personality;
      uint16_t slots_required;
      char description[ola::rdm::MAX_RDM_STRING_LENGTH];
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

RDMResponse *DummyRDMDevice::GetDmxStartAddress(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  uint16_t address = HostToNetwork(m_start_address);
  if (Footprint() == 0)
    address = 0xffff;
  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&address),
    sizeof(address));
}

RDMResponse *DummyRDMDevice::SetDmxStartAddress(
    const ola::rdm::RDMRequest *request) {
  uint16_t address;
  if (request->ParamDataSize() != sizeof(address)) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&address), request->ParamData(),
         sizeof(address));
  address = NetworkToHost(address);
  uint16_t end_address = DMX_UNIVERSE_SIZE - Footprint() + 1;
  if (address == 0 || address > end_address) {
    return NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  } else if (Footprint() == 0) {
    return NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  } else {
    m_start_address = address;
    return new ola::rdm::RDMSetResponse(
      request->DestinationUID(),
      request->SourceUID(),
      request->TransactionNumber(),
      ola::rdm::RDM_ACK,
      0,
      request->SubDevice(),
      request->ParamId(),
      NULL,
      0);
  }
}

RDMResponse *DummyRDMDevice::GetLampStrikes(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  uint32_t strikes = HostToNetwork(m_lamp_strikes);
  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&strikes),
    sizeof(strikes));
}

RDMResponse *DummyRDMDevice::SetLampStrikes(
    const ola::rdm::RDMRequest *request) {
  uint32_t lamp_strikes;
  if (request->ParamDataSize() != sizeof(lamp_strikes)) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  memcpy(reinterpret_cast<uint8_t*>(&lamp_strikes), request->ParamData(),
         sizeof(lamp_strikes));
  m_lamp_strikes = HostToNetwork(lamp_strikes);
  return new ola::rdm::RDMSetResponse(
    request->DestinationUID(),
    request->SourceUID(),
    request->TransactionNumber(),
    ola::rdm::RDM_ACK,
    0,
    request->SubDevice(),
    request->ParamId(),
    NULL,
    0);
}

RDMResponse *DummyRDMDevice::GetIdentify(const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  return GetResponseFromData(
      request,
      &m_identify_mode,
      sizeof(m_identify_mode));
}

RDMResponse *DummyRDMDevice::SetIdentify(const ola::rdm::RDMRequest *request) {
  uint8_t mode;
  if (request->ParamDataSize() != sizeof(mode)) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }

  mode = *request->ParamData();
  if (mode == 0 || mode == 1) {
    m_identify_mode = mode;
    OLA_INFO << "Dummy device, identify mode "
             << (m_identify_mode ? "on" : "off");
    return new ola::rdm::RDMSetResponse(
      request->DestinationUID(),
      request->SourceUID(),
      request->TransactionNumber(),
      ola::rdm::RDM_ACK,
      0,
      request->SubDevice(),
      request->ParamId(),
      NULL,
      0);
  } else {
    return NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  }
}

RDMResponse *DummyRDMDevice::GetRealTimeClock(
    const ola::rdm::RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
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


RDMResponse *DummyRDMDevice::GetManufacturerLabel(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, "Open Lighting Project");
}

RDMResponse *DummyRDMDevice::GetDeviceLabel(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, "Dummy RDM Device");
}

RDMResponse *DummyRDMDevice::GetDeviceModelDescription(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, "Dummy Model");
}

RDMResponse *DummyRDMDevice::GetSoftwareVersionLabel(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, "Dummy Software Version");
}

RDMResponse *DummyRDMDevice::GetOlaCodeVersion(
    const ola::rdm::RDMRequest *request) {
  return HandleStringResponse(request, VERSION);
}

/*
 * Handle a request that returns a string
 */
RDMResponse *DummyRDMDevice::HandleStringResponse(
    const ola::rdm::RDMRequest *request,
    const std::string &value) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, ola::rdm::NR_FORMAT_ERROR);
  }
  return GetResponseFromData(
        request,
        reinterpret_cast<const uint8_t*>(value.data()),
        value.size());
}
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
