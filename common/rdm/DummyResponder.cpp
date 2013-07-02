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
 * DummyResponder.cpp
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
#include "ola/rdm/DummyResponder.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

DummyResponder::RDMOps *DummyResponder::RDMOps::instance = NULL;

const ResponderOps<DummyResponder>::ParamHandler
    DummyResponder::PARAM_HANDLERS[] = {
  { PID_PARAMETER_DESCRIPTION,
    &DummyResponder::GetParamDescription,
    NULL},
  { PID_DEVICE_INFO,
    &DummyResponder::GetDeviceInfo,
    NULL},
  { PID_PRODUCT_DETAIL_ID_LIST,
    &DummyResponder::GetProductDetailList,
    NULL},
  { PID_DEVICE_MODEL_DESCRIPTION,
    &DummyResponder::GetDeviceModelDescription,
    NULL},
  { PID_MANUFACTURER_LABEL,
    &DummyResponder::GetManufacturerLabel,
    NULL},
  { PID_DEVICE_LABEL,
    &DummyResponder::GetDeviceLabel,
    NULL},
  { PID_FACTORY_DEFAULTS,
    &DummyResponder::GetFactoryDefaults,
    &DummyResponder::SetFactoryDefaults},
  { PID_SOFTWARE_VERSION_LABEL,
    &DummyResponder::GetSoftwareVersionLabel,
    NULL},
  { PID_DMX_PERSONALITY,
    &DummyResponder::GetPersonality,
    &DummyResponder::SetPersonality},
  { PID_DMX_PERSONALITY_DESCRIPTION,
    &DummyResponder::GetPersonalityDescription,
    NULL},
  { PID_DMX_START_ADDRESS,
    &DummyResponder::GetDmxStartAddress,
    &DummyResponder::SetDmxStartAddress},
  { PID_LAMP_STRIKES,
    &DummyResponder::GetLampStrikes,
    &DummyResponder::SetLampStrikes},
  { PID_IDENTIFY_DEVICE,
    &DummyResponder::GetIdentify,
    &DummyResponder::SetIdentify},
  { PID_REAL_TIME_CLOCK,
    &DummyResponder::GetRealTimeClock,
    NULL},
  { OLA_MANUFACTURER_PID_CODE_VERSION,
    &DummyResponder::GetOlaCodeVersion,
    NULL},
  { 0, NULL, NULL},
};

const DummyResponder::personality_info DummyResponder::PERSONALITIES[] = {
  {0, "Personality 1"},
  {5, "Personality 2"},
  {10, "Personality 3"},
  {20, "Personality 4"},
};

/*
 * Handle an RDM Request
 */
void DummyResponder::SendRDMRequest(const RDMRequest *request,
                                    RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ola::rdm::ROOT_RDM_DEVICE,
                                       request, callback);
}

const RDMResponse *DummyResponder::GetParamDescription(
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

const RDMResponse *DummyResponder::GetDeviceInfo(const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  return ResponderHelper::GetDeviceInfo(
      request, OLA_DUMMY_DEVICE_MODEL, PRODUCT_CATEGORY_OTHER, 1, Footprint(),
      m_personality + 1, arraysize(PERSONALITIES),
      Footprint() ? m_start_address : 0xffff, 0, 0);
}

/**
 * Reset to factory defaults
 */
const RDMResponse *DummyResponder::GetFactoryDefaults(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint8_t using_defaults = (m_start_address == 1 && m_personality == 1 &&
                            m_identify_mode == false);
  return GetResponseFromData(request, &using_defaults, sizeof(using_defaults));
}

const RDMResponse *DummyResponder::SetFactoryDefaults(
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

const RDMResponse *DummyResponder::GetProductDetailList(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint16_t product_details[] = {
    PRODUCT_DETAIL_TEST,
    PRODUCT_DETAIL_OTHER
  };

  for (unsigned int i = 0; i < arraysize(product_details); i++)
    product_details[i] = HostToNetwork(product_details[i]);

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&product_details),
      sizeof(product_details));
}

const RDMResponse *DummyResponder::GetPersonality(const RDMRequest *request) {
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

const RDMResponse *DummyResponder::SetPersonality(const RDMRequest *request) {
  uint8_t personality;
  if (!ResponderHelper::ExtractUInt8(request, &personality)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

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

const RDMResponse *DummyResponder::GetPersonalityDescription(
    const RDMRequest *request) {
  uint8_t personality;
  if (!ResponderHelper::ExtractUInt8(request, &personality)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }
  personality-= 1;
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

const RDMResponse *DummyResponder::GetDmxStartAddress(
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

const RDMResponse *DummyResponder::SetDmxStartAddress(
    const RDMRequest *request) {
  uint16_t address;
  if (!ResponderHelper::ExtractUInt16(request, &address)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

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

const RDMResponse *DummyResponder::GetLampStrikes(const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint32_t strikes = HostToNetwork(m_lamp_strikes);
  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&strikes),
    sizeof(strikes));
}

const RDMResponse *DummyResponder::SetLampStrikes(const RDMRequest *request) {
  uint32_t lamp_strikes;
  if (!ResponderHelper::ExtractUInt32(request, &lamp_strikes)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

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

const RDMResponse *DummyResponder::GetIdentify(const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

const RDMResponse *DummyResponder::SetIdentify(const RDMRequest *request) {
  bool old_value = m_identify_mode;
  const RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "Dummy device, identify mode "
             << (m_identify_mode ? "on" : "off");
  }
  return response;
}

const RDMResponse *DummyResponder::GetRealTimeClock(const RDMRequest *request) {
  return ResponderHelper::GetRealTimeClock(request);
}

const RDMResponse *DummyResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Open Lighting Project");
}

const RDMResponse *DummyResponder::GetDeviceLabel(const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy RDM Device");
}

const RDMResponse *DummyResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy Model");
}

const RDMResponse *DummyResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy Software Version");
}

const RDMResponse *DummyResponder::GetOlaCodeVersion(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, VERSION);
}
}  // namespace rdm
}  // namespace ola
