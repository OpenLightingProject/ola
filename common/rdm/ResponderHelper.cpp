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
 * ResponderHelper.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/RDMEnums.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

template<typename T>
static bool GenericExtractValue(const RDMRequest *request, T *output) {
  T value;
  if (request->ParamDataSize() != sizeof(value)) {
    return false;
  }

  memcpy(reinterpret_cast<uint8_t*>(&value), request->ParamData(),
         sizeof(value));
  *output = NetworkToHost(value);
  return true;
}

bool ResponderHelper::ExtractUInt8(const RDMRequest *request,
                                   uint8_t *output) {
  return GenericExtractValue(request, output);
}

bool ResponderHelper::ExtractUInt16(const RDMRequest *request,
                                    uint16_t *output) {
  return GenericExtractValue(request, output);
}

bool ResponderHelper::ExtractUInt32(const RDMRequest *request,
                                    uint32_t *output) {
  return GenericExtractValue(request, output);
}


const RDMResponse *ResponderHelper::GetDeviceInfo(
    const RDMRequest *request,
    uint16_t device_model,
    rdm_product_category product_category,
    uint32_t software_version,
    uint16_t dmx_footprint,
    uint8_t current_personality,
    uint8_t personality_count,
    uint16_t dmx_start_address,
    uint16_t sub_device_count,
    uint8_t sensor_count,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
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
  device_info.rdm_version = HostToNetwork(
      static_cast<uint16_t>(RDM_VERSION_1_0));
  device_info.model = HostToNetwork(device_model);
  device_info.product_category = HostToNetwork(
      static_cast<uint16_t>(product_category));
  device_info.software_version = HostToNetwork(software_version);
  device_info.dmx_footprint = HostToNetwork(dmx_footprint);
  device_info.current_personality = current_personality;
  device_info.personality_count = personality_count;
  device_info.dmx_start_address = HostToNetwork(dmx_start_address);
  device_info.sub_device_count = HostToNetwork(sub_device_count);
  device_info.sensor_count = sensor_count;
  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&device_info),
      sizeof(device_info),
      RDM_ACK,
      queued_message_count);
}

const RDMResponse *ResponderHelper::GetDeviceInfo(
    const RDMRequest *request,
    uint16_t device_model,
    rdm_product_category product_category,
    uint32_t software_version,
    const PersonalityManager *personality_manager,
    uint16_t start_address,
    uint16_t sub_device_count,
    uint8_t sensor_count,
    uint8_t queued_message_count) {
  return ResponderHelper::GetDeviceInfo(
      request, device_model, product_category, software_version,
      personality_manager->ActivePersonalityFootprint(),
      personality_manager->ActivePersonalityNumber(),
      personality_manager->PersonalityCount(),
      (personality_manager->ActivePersonalityFootprint() ?
        start_address : ZERO_FOOTPRINT_DMX_ADDRESS),
      sub_device_count, sensor_count, queued_message_count);
}

const RDMResponse *ResponderHelper::GetProductDetailList(
    const RDMRequest *request,
    const std::vector<rdm_product_detail> &product_details,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  uint16_t product_details_raw[product_details.size()];

  for (unsigned int i = 0; i < product_details.size(); i++) {
    product_details_raw[i] =
      HostToNetwork(static_cast<uint16_t>(product_details[i]));
  }

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&product_details_raw),
      sizeof(product_details_raw),
      RDM_ACK,
      queued_message_count);
}

const RDMResponse *ResponderHelper::GetPersonality(
    const RDMRequest *request,
    const PersonalityManager *personality_manager,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  struct personality_info_s {
    uint8_t personality;
    uint8_t total;
  } __attribute__((packed));

  struct personality_info_s personality_info = {
      personality_manager->ActivePersonalityNumber(),
      personality_manager->PersonalityCount()
  };
  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&personality_info),
    sizeof(personality_info),
    RDM_ACK,
    queued_message_count);
}

const RDMResponse *ResponderHelper::SetPersonality(
    const RDMRequest *request,
    PersonalityManager *personality_manager,
    uint16_t start_address,
    uint8_t queued_message_count) {
  uint8_t personality_number;
  if (!ExtractUInt8(request, &personality_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  const Personality *personality = personality_manager->Lookup(
    personality_number);

  if (!personality) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, queued_message_count);
  } else if (start_address + personality->Footprint() - 1 > DMX_UNIVERSE_SIZE) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, queued_message_count);
  } else {
    personality_manager->SetActivePersonality(personality_number);
    return EmptySetResponse(request, queued_message_count);
  }
}

const RDMResponse *ResponderHelper::GetPersonalityDescription(
    const RDMRequest *request,
    const PersonalityManager *personality_manager,
    uint8_t queued_message_count) {
  uint8_t personality_number;
  if (!ExtractUInt8(request, &personality_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  const Personality *personality = personality_manager->Lookup(
    personality_number);

  if (!personality) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, queued_message_count);
  } else {
    struct personality_description_s {
      uint8_t personality;
      uint16_t slots_required;
      char description[MAX_RDM_STRING_LENGTH];
    } __attribute__((packed));

    struct personality_description_s personality_description;
    personality_description.personality = personality_number;
    personality_description.slots_required =
        HostToNetwork(personality->Footprint());
    strncpy(personality_description.description,
            personality->Description().c_str(),
            sizeof(personality_description.description));

    return GetResponseFromData(
        request,
        reinterpret_cast<uint8_t*>(&personality_description),
        sizeof(personality_description),
        RDM_ACK,
        queued_message_count);
  }
}


/**
 * Get slot info
 */
const RDMResponse *ResponderHelper::GetSlotInfo(
    const RDMRequest *request,
    const PersonalityManager *personality_manager,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  OLA_DEBUG << "Looking for slot info";
  const SlotDataCollection *slot_data_collection =
      personality_manager->ActivePersonality()->GetAllSlotData();
  OLA_DEBUG << "Got slot data collection with count " <<
      slot_data_collection->SlotDataCount();

  if (slot_data_collection->SlotDataCount() <= 0) {
    return EmptyGetResponse(request, queued_message_count);
  }

  struct slot_info_s {
    uint16_t offset;
    uint8_t type;
    uint16_t label;
  } __attribute__((packed));

  slot_info_s slot_info_raw[slot_data_collection->SlotDataCount()];

  for (uint16_t slot = 0;
       slot < slot_data_collection->SlotDataCount();
       slot++) {
    const SlotData *sd = slot_data_collection->Lookup(slot);
    OLA_DEBUG << "Processing slot " << slot << " type " <<
        static_cast<int>(sd->SlotType()) << ", definition " <<
        static_cast<uint16_t>(sd->SlotDefinition());
    slot_info_raw[slot].offset = HostToNetwork(slot);
    slot_info_raw[slot].type = static_cast<uint8_t>(sd->SlotType());
    slot_info_raw[slot].label =
        HostToNetwork(static_cast<uint16_t>(sd->SlotDefinition()));
  }

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&slot_info_raw),
      sizeof(slot_info_raw),
      RDM_ACK,
      queued_message_count);
}


/**
 * Get a slot description
 */
const RDMResponse *ResponderHelper::GetSlotDescription(
    const RDMRequest *request,
    const PersonalityManager *personality_manager,
    uint8_t queued_message_count) {
  uint16_t slot_number;
  if (!ExtractUInt16(request, &slot_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  OLA_DEBUG << "Looking for slot desc for slot " << slot_number;
  const SlotData *slot_data =
      personality_manager->ActivePersonality()->GetSlotData(slot_number);

  if (!slot_data) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, queued_message_count);
  } else {
    OLA_DEBUG << "Got slot description " << slot_data->Description() <<
        " for slot " << slot_number;
    if (slot_data->Description().empty())
      return NackWithReason(request,
                            NR_DATA_OUT_OF_RANGE,
                            queued_message_count);

    struct slot_description_s {
      uint16_t slot;
      char description[MAX_RDM_STRING_LENGTH];
    } __attribute__((packed));

    struct slot_description_s slot_description;
    slot_description.slot = HostToNetwork(slot_number);
    strncpy(slot_description.description,
            slot_data->Description().c_str(),
            sizeof(slot_description.description));

    return GetResponseFromData(request,
                               reinterpret_cast<uint8_t*>(&slot_description),
                               sizeof(slot_description),
                               RDM_ACK,
                               queued_message_count);
  }
}


/**
 * Get slot default values
 */
const RDMResponse *ResponderHelper::GetSlotDefaultValues(
    const RDMRequest *request,
    const PersonalityManager *personality_manager,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  OLA_DEBUG << "Looking for slot defaults";
  const SlotDataCollection *slot_data_collection =
      personality_manager->ActivePersonality()->GetAllSlotData();
  OLA_DEBUG << "Got slot data collection with count " <<
      slot_data_collection->SlotDataCount();

  if (slot_data_collection->SlotDataCount() <= 0) {
    return EmptyGetResponse(request, queued_message_count);
  }

  struct slot_default_s {
    uint16_t offset;
    uint8_t value;
  } __attribute__((packed));

  slot_default_s slot_default_raw[slot_data_collection->SlotDataCount()];

  for (uint16_t slot = 0;
       slot < slot_data_collection->SlotDataCount();
       slot++) {
    const SlotData *sd = slot_data_collection->Lookup(slot);
    OLA_DEBUG << "Processing slot " << slot << " default " <<
        static_cast<int>(sd->DefaultSlotValue());
    slot_default_raw[slot].offset = HostToNetwork(slot);
    slot_default_raw[slot].value =
        static_cast<uint8_t>(sd->DefaultSlotValue());
  }

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&slot_default_raw),
      sizeof(slot_default_raw),
      RDM_ACK,
      queued_message_count);
}


/**
 * Get the start address
 */
const RDMResponse *ResponderHelper::GetDmxAddress(
    const RDMRequest *request,
    const PersonalityManager *personality_manager,
    uint16_t start_address,
    uint8_t queued_message_count) {
  return ResponderHelper::GetUInt16Value(
    request,
    ((personality_manager->ActivePersonalityFootprint() == 0) ?
        ZERO_FOOTPRINT_DMX_ADDRESS : start_address),
    queued_message_count);
}

/**
 * Set the start address
 */
const RDMResponse *ResponderHelper::SetDmxAddress(
    const RDMRequest *request,
    const PersonalityManager *personality_manager,
    uint16_t *dmx_address,
    uint8_t queued_message_count) {
  uint16_t address;
  if (!ResponderHelper::ExtractUInt16(request, &address)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint16_t end_address = (1 + DMX_UNIVERSE_SIZE -
                          personality_manager->ActivePersonalityFootprint());
  if (address == 0 || address > end_address) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, queued_message_count);
  } else if (personality_manager->ActivePersonalityFootprint() == 0) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, queued_message_count);
  } else {
    *dmx_address = address;
    return EmptySetResponse(request, queued_message_count);
  }
}

/**
 * Get the clock response.
 */
const RDMResponse *ResponderHelper::GetRealTimeClock(
    const RDMRequest *request,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
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
      sizeof(clock),
      RDM_ACK,
      queued_message_count);
}

const RDMResponse *ResponderHelper::GetParamDescription(
    const RDMRequest *request,
    uint16_t pid,
    uint8_t pdl_size,
    rdm_data_type data_type,
    rdm_command_class command_class,
    rdm_pid_unit unit,
    rdm_pid_prefix prefix,
    uint32_t min_value,
    uint32_t default_value,
    uint32_t max_value,
    string description,
    uint8_t queued_message_count) {
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
  param_description.pid = HostToNetwork(pid);
  param_description.pdl_size = HostToNetwork(pdl_size);
  param_description.data_type = HostToNetwork(
      static_cast<uint8_t>(data_type));
  param_description.command_class = HostToNetwork(
      static_cast<uint8_t>(command_class));
  param_description.type = 0;
  param_description.unit = HostToNetwork(
      static_cast<uint8_t>(unit));
  param_description.prefix = HostToNetwork(
      static_cast<uint8_t>(prefix));
  param_description.min_value = min_value;
  param_description.default_value = default_value;
  param_description.max_value = max_value;
  strncpy(param_description.description, description.c_str(),
          MAX_RDM_STRING_LENGTH);
  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&param_description),
      sizeof(param_description),
      RDM_ACK,
      queued_message_count);
}

const RDMResponse *ResponderHelper::GetASCIIParamDescription(
        const RDMRequest *request,
        uint16_t pid,
        rdm_command_class command_class,
        string description,
        uint8_t queued_message_count) {
  return GetParamDescription(
      request,
      pid,
      static_cast<uint8_t>(MAX_RDM_STRING_LENGTH),
      DS_ASCII,
      command_class,
      UNITS_NONE,
      PREFIX_NONE,
      static_cast<uint32_t>(0),
      static_cast<uint32_t>(0),
      static_cast<uint32_t>(0),
      description,
      queued_message_count);
}

const RDMResponse *ResponderHelper::GetBitFieldParamDescription(
        const RDMRequest *request,
        uint16_t pid,
        uint8_t pdl_size,
        rdm_command_class command_class,
        string description,
        uint8_t queued_message_count) {
  return GetParamDescription(
      request,
      pid,
      pdl_size,
      DS_BIT_FIELD,
      command_class,
      UNITS_NONE,
      PREFIX_NONE,
      static_cast<uint32_t>(0),
      static_cast<uint32_t>(0),
      static_cast<uint32_t>(0),
      description,
      queued_message_count);
}

/*
 * Handle a request that returns a string
 */
const RDMResponse *ResponderHelper::GetString(
    const RDMRequest *request,
    const std::string &value,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  return GetResponseFromData(request,
                             reinterpret_cast<const uint8_t*>(value.data()),
                             value.size(),
                             RDM_ACK,
                             queued_message_count);
}

const RDMResponse *ResponderHelper::EmptyGetResponse(
    const RDMRequest *request,
    uint8_t queued_message_count) {
  return GetResponseFromData(request,
                             NULL,
                             0,
                             RDM_ACK,
                             queued_message_count);
}

const RDMResponse *ResponderHelper::EmptySetResponse(
    const RDMRequest *request,
    uint8_t queued_message_count) {
  return new RDMSetResponse(
    request->DestinationUID(),
    request->SourceUID(),
    request->TransactionNumber(),
    RDM_ACK,
    queued_message_count,
    request->SubDevice(),
    request->ParamId(),
    NULL,
    0);
}

const RDMResponse *ResponderHelper::SetString(
    const RDMRequest *request,
    std::string *value,
    uint8_t queued_message_count) {
  if (request->ParamDataSize() > MAX_RDM_STRING_LENGTH) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  const string new_label(reinterpret_cast<const char*>(request->ParamData()),
                         request->ParamDataSize());
  *value = new_label;
  return EmptySetResponse(request, queued_message_count);
}

const RDMResponse *ResponderHelper::GetBoolValue(const RDMRequest *request,
                                                 bool value,
                                                 uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  uint8_t param = value ? 1 : 0;

  return GetResponseFromData(request, &param, sizeof(param),
                             RDM_ACK, queued_message_count);
}

const RDMResponse *ResponderHelper::SetBoolValue(const RDMRequest *request,
                                                 bool *value,
                                                 uint8_t queued_message_count) {
  uint8_t arg;
  if (!ResponderHelper::ExtractUInt8(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  if (arg == 0 || arg == 1) {
    *value = arg;
    return EmptySetResponse(request, queued_message_count);
  } else {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, queued_message_count);
  }
}

template<typename T>
static const RDMResponse *GenericGetIntValue(const RDMRequest *request,
                                             T value,
                                             uint8_t queued_message_count = 0) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  T param = HostToNetwork(value);
  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&param),
    sizeof(param),
    RDM_ACK,
    queued_message_count);
}

const RDMResponse *ResponderHelper::GetUInt8Value(
    const RDMRequest *request,
    uint8_t value,
    uint8_t queued_message_count) {
  return GenericGetIntValue(request, value, queued_message_count);
}

const RDMResponse *ResponderHelper::GetUInt16Value(
    const RDMRequest *request,
    uint16_t value,
    uint8_t queued_message_count) {
  return GenericGetIntValue(request, value, queued_message_count);
}

const RDMResponse *ResponderHelper::GetUInt32Value(
    const RDMRequest *request,
    uint32_t value,
    uint8_t queued_message_count) {
  return GenericGetIntValue(request, value, queued_message_count);
}

template<typename T>
static const RDMResponse *GenericSetIntValue(const RDMRequest *request,
                                             T *value,
                                             uint8_t queued_message_count = 0) {
  if (!GenericExtractValue(request, value)) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  return ResponderHelper::EmptySetResponse(request, queued_message_count);
}

const RDMResponse *ResponderHelper::SetUInt8Value(
    const RDMRequest *request,
    uint8_t *value,
    uint8_t queued_message_count) {
  return GenericSetIntValue(request, value, queued_message_count);
}

const RDMResponse *ResponderHelper::SetUInt16Value(
    const RDMRequest *request,
    uint16_t *value,
    uint8_t queued_message_count) {
  return GenericSetIntValue(request, value, queued_message_count);
}

const RDMResponse *ResponderHelper::SetUInt32Value(
    const RDMRequest *request,
    uint32_t *value,
    uint8_t queued_message_count) {
  return GenericSetIntValue(request, value, queued_message_count);
}
}  // namespace rdm
}  // namespace ola
