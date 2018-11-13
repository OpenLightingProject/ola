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
 * ResponderHelper.cpp
 * Copyright (C) 2013 Simon Newton
 */

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends
#include <stdint.h>

#include <algorithm>
#include <string>
#include <vector>
#include "ola/Clock.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/base/Macro.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Interface.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/MACAddress.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/ResponderSensor.h"
#include "ola/strings/Utils.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::Interface;
using ola::network::InterfacePicker;
using ola::network::IPV4Address;
using ola::network::MACAddress;
using ola::network::NetworkToHost;
using std::min;
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


RDMResponse *ResponderHelper::GetDeviceInfo(
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

  PACK(
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
  });
  STATIC_ASSERT(sizeof(device_info_s) == 19);

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

RDMResponse *ResponderHelper::GetDeviceInfo(
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

RDMResponse *ResponderHelper::GetProductDetailList(
    const RDMRequest *request,
    const vector<rdm_product_detail> &product_details,
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

RDMResponse *ResponderHelper::GetPersonality(
    const RDMRequest *request,
    const PersonalityManager *personality_manager,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  PACK(
  struct personality_info_s {
    uint8_t personality;
    uint8_t total;
  });
  STATIC_ASSERT(sizeof(personality_info_s) == 2);

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

RDMResponse *ResponderHelper::SetPersonality(
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

RDMResponse *ResponderHelper::GetPersonalityDescription(
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
    PACK(
    struct personality_description_s {
      uint8_t personality;
      uint16_t slots_required;
      char description[MAX_RDM_STRING_LENGTH];
    });
    STATIC_ASSERT(sizeof(personality_description_s) == 35);

    struct personality_description_s personality_description;
    personality_description.personality = personality_number;
    personality_description.slots_required =
        HostToNetwork(personality->Footprint());

    size_t str_len = min(personality->Description().size(),
                         sizeof(personality_description.description));
    strncpy(personality_description.description,
            personality->Description().c_str(), str_len);

    unsigned int param_data_size = (
        sizeof(personality_description) -
        sizeof(personality_description.description) + str_len);

    return GetResponseFromData(
        request,
        reinterpret_cast<uint8_t*>(&personality_description),
        param_data_size,
        RDM_ACK,
        queued_message_count);
  }
}


/**
 * Get slot info
 */
RDMResponse *ResponderHelper::GetSlotInfo(
    const RDMRequest *request,
    const PersonalityManager *personality_manager,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  const SlotDataCollection *slot_data =
      personality_manager->ActivePersonality()->GetSlotData();

  if (slot_data->SlotCount() == 0) {
    return EmptyGetResponse(request, queued_message_count);
  }

  PACK(
  struct slot_info_s {
    uint16_t offset;
    uint8_t type;
    uint16_t label;
  });
  STATIC_ASSERT(sizeof(slot_info_s) == 5);

  slot_info_s slot_info_raw[slot_data->SlotCount()];

  for (uint16_t slot = 0; slot < slot_data->SlotCount(); slot++) {
    const SlotData *sd = slot_data->Lookup(slot);
    slot_info_raw[slot].offset = HostToNetwork(slot);
    slot_info_raw[slot].type = static_cast<uint8_t>(sd->SlotType());
    slot_info_raw[slot].label = HostToNetwork(sd->SlotIDDefinition());
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
RDMResponse *ResponderHelper::GetSlotDescription(
    const RDMRequest *request,
    const PersonalityManager *personality_manager,
    uint8_t queued_message_count) {
  uint16_t slot_number;
  if (!ExtractUInt16(request, &slot_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  const SlotData *slot_data =
      personality_manager->ActivePersonality()->GetSlotData(slot_number);

  if (!slot_data) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, queued_message_count);
  }

  if (!slot_data->HasDescription()) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE, queued_message_count);
  }

  PACK(
  struct slot_description_s {
    uint16_t slot;
    char description[MAX_RDM_STRING_LENGTH];
  });
  STATIC_ASSERT(sizeof(slot_description_s) == 34);

  struct slot_description_s slot_description;
  slot_description.slot = HostToNetwork(slot_number);

  size_t str_len = min(slot_data->Description().size(),
                       sizeof(slot_description.description));
  strncpy(slot_description.description, slot_data->Description().c_str(),
          str_len);

  unsigned int param_data_size = (
      sizeof(slot_description) -
      sizeof(slot_description.description) + str_len);

  return GetResponseFromData(request,
                             reinterpret_cast<uint8_t*>(&slot_description),
                             param_data_size,
                             RDM_ACK,
                             queued_message_count);
}


/**
 * Get slot default values
 */
RDMResponse *ResponderHelper::GetSlotDefaultValues(
    const RDMRequest *request,
    const PersonalityManager *personality_manager,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  const SlotDataCollection *slot_data =
      personality_manager->ActivePersonality()->GetSlotData();

  if (slot_data->SlotCount() == 0) {
    return EmptyGetResponse(request, queued_message_count);
  }

  PACK(
  struct slot_default_s {
    uint16_t offset;
    uint8_t value;
  });
  STATIC_ASSERT(sizeof(slot_default_s) == 3);

  slot_default_s slot_default_raw[slot_data->SlotCount()];

  for (uint16_t slot = 0; slot < slot_data->SlotCount(); slot++) {
    const SlotData *sd = slot_data->Lookup(slot);
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
RDMResponse *ResponderHelper::GetDmxAddress(
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
RDMResponse *ResponderHelper::SetDmxAddress(
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
 * Get a sensor definition
 */
RDMResponse *ResponderHelper::GetSensorDefinition(
    const RDMRequest *request, const Sensors &sensor_list) {
  uint8_t sensor_number;
  if (!ResponderHelper::ExtractUInt8(request, &sensor_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (sensor_number >= sensor_list.size()) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  PACK(
  struct sensor_definition_s {
    uint8_t sensor;
    uint8_t type;
    uint8_t unit;
    uint8_t prefix;
    int16_t range_min;
    int16_t range_max;
    int16_t normal_min;
    int16_t normal_max;
    uint8_t recorded_support;
    char description[MAX_RDM_STRING_LENGTH];
  });
  STATIC_ASSERT(sizeof(sensor_definition_s) == 45);

  const Sensor *sensor = sensor_list.at(sensor_number);
  struct sensor_definition_s sensor_definition;
  sensor_definition.sensor = sensor_number;
  sensor_definition.type = sensor->Type();
  sensor_definition.unit = sensor->Unit();
  sensor_definition.prefix = sensor->Prefix();
  sensor_definition.range_min = HostToNetwork(sensor->RangeMin());
  sensor_definition.range_max = HostToNetwork(sensor->RangeMax());
  sensor_definition.normal_min = HostToNetwork(sensor->NormalMin());
  sensor_definition.normal_max = HostToNetwork(sensor->NormalMax());
  sensor_definition.recorded_support = sensor->RecordedSupportBitMask();
  strings::CopyToFixedLengthBuffer(sensor->Description(),
                                   sensor_definition.description,
                                   arraysize(sensor_definition.description));
  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&sensor_definition),
    sizeof(sensor_definition));
}

/**
 * Get a sensor value
 */
RDMResponse *ResponderHelper::GetSensorValue(
    const RDMRequest *request, const Sensors &sensor_list) {
  uint8_t sensor_number;
  if (!ResponderHelper::ExtractUInt8(request, &sensor_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (sensor_number >= sensor_list.size()) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  Sensor *sensor = sensor_list.at(sensor_number);
  struct sensor_value_s sensor_value = {
    sensor_number,
    HostToNetwork(sensor->FetchValue()),
    HostToNetwork(sensor->Lowest()),
    HostToNetwork(sensor->Highest()),
    HostToNetwork(sensor->Recorded()),
  };

  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&sensor_value),
    sizeof(sensor_value));
}

/**
 * Set a sensor value
 */
RDMResponse *ResponderHelper::SetSensorValue(
    const RDMRequest *request, const Sensors &sensor_list) {
  uint8_t sensor_number;
  if (!ResponderHelper::ExtractUInt8(request, &sensor_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  int16_t value = 0;
  if (sensor_number == ALL_SENSORS) {
    Sensors::const_iterator iter = sensor_list.begin();
    for (; iter != sensor_list.end(); ++iter) {
      value = (*iter)->Reset();
    }
  } else if (sensor_number < sensor_list.size()) {
    Sensor *sensor = sensor_list.at(sensor_number);
    value = sensor->Reset();
  } else {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  struct sensor_value_s sensor_value = {
    sensor_number,
    HostToNetwork(value),
    HostToNetwork(value),
    HostToNetwork(value),
    HostToNetwork(value),
  };

  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&sensor_value),
    sizeof(sensor_value));
}


/**
 * Record a sensor
 */
RDMResponse *ResponderHelper::RecordSensor(
    const RDMRequest *request, const Sensors &sensor_list) {
  uint8_t sensor_number;
  if (!ResponderHelper::ExtractUInt8(request, &sensor_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if ((sensor_number == ALL_SENSORS) && (sensor_list.size() > 0)) {
    Sensors::const_iterator iter = sensor_list.begin();
    for (; iter != sensor_list.end(); ++iter) {
      (*iter)->Record();
    }
  } else if (sensor_number < sensor_list.size()) {
    Sensor *sensor = sensor_list.at(sensor_number);
    sensor->Record();
  } else {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  return GetResponseFromData(request, NULL, 0);
}

/**
 * Get the clock response.
 */
RDMResponse *ResponderHelper::GetRealTimeClock(
    const RDMRequest *request,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  PACK(
  struct clock_s {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  });
  STATIC_ASSERT(sizeof(clock_s) == 7);

  time_t now;
  now = time(NULL);
  struct tm tm_now;
#ifdef _WIN32
  tm_now = *localtime(&now);  // NOLINT(runtime/threadsafe_fn)
#else
  localtime_r(&now, &tm_now);
#endif  // _WIN32

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

RDMResponse *ResponderHelper::GetListInterfaces(
    const RDMRequest *request,
    const NetworkManagerInterface *network_manager,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  vector<Interface> interfaces =
      network_manager->GetInterfacePicker()->GetInterfaces(false);

  if (interfaces.size() == 0) {
    return EmptyGetResponse(request, queued_message_count);
  }

  std::sort(interfaces.begin(), interfaces.end(),
            ola::network::InterfaceIndexOrdering());

  uint16_t interface_count = std::count_if(
      interfaces.begin(), interfaces.end(), IsInterfaceIndexValidInterface);

  PACK(
  struct list_interfaces_s {
    uint32_t index;
    uint16_t type;
  });
  STATIC_ASSERT(sizeof(list_interfaces_s) == 6);

  list_interfaces_s list_interfaces[interface_count];

  // Reorder so valid interfaces are first
  std::stable_partition(interfaces.begin(), interfaces.end(),
                        IsInterfaceIndexValidInterface);

  // Then just iterate through the valid ones
  vector<Interface>::iterator iter = interfaces.begin();
  for (uint16_t i = 0; i < interface_count; i++) {
    list_interfaces[i].index = HostToNetwork(iter[i].index);
    list_interfaces[i].type = HostToNetwork(
        static_cast<uint16_t>(iter[i].type));
  }

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&list_interfaces),
      sizeof(list_interfaces),
      RDM_ACK,
      queued_message_count);
}


RDMResponse *ResponderHelper::GetInterfaceLabel(
    const RDMRequest *request,
    const NetworkManagerInterface *network_manager,
    uint8_t queued_message_count) {
  uint32_t index;
  if (!ResponderHelper::ExtractUInt32(request, &index)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  Interface interface;
  if (!FindInterface(network_manager, &interface, index)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  PACK(
  struct interface_label_s {
    uint32_t index;
    char label[MAX_RDM_STRING_LENGTH];
  });
  STATIC_ASSERT(sizeof(interface_label_s) == 36);

  struct interface_label_s interface_label;
  interface_label.index = HostToNetwork(interface.index);

  size_t str_len = min(interface.name.size(), sizeof(interface_label.label));
  strncpy(interface_label.label, interface.name.c_str(), str_len);

  unsigned int param_data_size = (
      sizeof(interface_label) -
      sizeof(interface_label.label) + str_len);

  return GetResponseFromData(request,
                             reinterpret_cast<uint8_t*>(&interface_label),
                             param_data_size,
                             RDM_ACK,
                             queued_message_count);
}

RDMResponse *ResponderHelper::GetInterfaceHardwareAddressType1(
    const RDMRequest *request,
    const NetworkManagerInterface *network_manager,
    uint8_t queued_message_count) {
  uint32_t index;
  if (!ResponderHelper::ExtractUInt32(request, &index)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  Interface interface;
  if (!FindInterface(network_manager, &interface, index)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  // Only return type 1 (Ethernet)
  if (interface.type != Interface::ARP_ETHERNET_TYPE) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  PACK(
  struct interface_hardware_address_s {
    uint32_t index;
    uint8_t hardware_address[MACAddress::LENGTH];
  });
  STATIC_ASSERT(sizeof(interface_hardware_address_s) == 10);

  struct interface_hardware_address_s interface_hardware_address;
  interface_hardware_address.index = HostToNetwork(interface.index);
  interface.hw_address.Get(interface_hardware_address.hardware_address);

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&interface_hardware_address),
      sizeof(interface_hardware_address),
      RDM_ACK,
      queued_message_count);
}

RDMResponse *ResponderHelper::GetIPV4CurrentAddress(
    const RDMRequest *request,
    const NetworkManagerInterface *network_manager,
    uint8_t queued_message_count) {
  uint32_t index;
  if (!ResponderHelper::ExtractUInt32(request, &index)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  Interface interface;
  if (!FindInterface(network_manager, &interface, index)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  PACK(
  struct ipv4_current_address_s {
    uint32_t index;
    uint32_t ipv4_address;
    uint8_t netmask;
    uint8_t dhcp;
  });
  STATIC_ASSERT(sizeof(ipv4_current_address_s) == 10);

  struct ipv4_current_address_s ipv4_current_address;
  ipv4_current_address.index = HostToNetwork(interface.index);

  // Already in correct byte order
  ipv4_current_address.ipv4_address = interface.ip_address.AsInt();

  uint8_t mask = UINT8_MAX;
  if (!IPV4Address::ToCIDRMask(interface.subnet_mask, &mask)) {
    OLA_WARN << "Error converting " << interface.subnet_mask
             << " to CIDR value";
  }

  ipv4_current_address.netmask = mask;
  ipv4_current_address.dhcp = static_cast<uint8_t>(
      network_manager->GetDHCPStatus(interface));

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&ipv4_current_address),
      sizeof(ipv4_current_address),
      RDM_ACK,
      queued_message_count);
}


RDMResponse *ResponderHelper::GetIPV4DefaultRoute(
    const RDMRequest *request,
    const NetworkManagerInterface *network_manager,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  int32_t if_index = Interface::DEFAULT_INDEX;
  IPV4Address default_route;
  if (!network_manager->GetIPV4DefaultRoute(&if_index, &default_route)) {
    return NackWithReason(request, NR_HARDWARE_FAULT);
  }

  PACK(
  struct ipv4_default_route_s {
    uint32_t if_index;
    uint32_t default_route;
  });
  STATIC_ASSERT(sizeof(ipv4_default_route_s) == 8);

  struct ipv4_default_route_s ipv4_default_route;

  if (if_index == Interface::DEFAULT_INDEX) {
    // No default route interface index set, return special value
    ipv4_default_route.if_index = HostToNetwork(NO_DEFAULT_ROUTE);
  } else {
    ipv4_default_route.if_index = HostToNetwork(if_index);
  }

  if (default_route.IsWildcard()) {
    // No default route set, return special value
    ipv4_default_route.default_route = HostToNetwork(NO_DEFAULT_ROUTE);
  } else {
    // Already in correct byte order
    ipv4_default_route.default_route = default_route.AsInt();
  }

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&ipv4_default_route),
      sizeof(ipv4_default_route),
      RDM_ACK,
      queued_message_count);
}


RDMResponse *ResponderHelper::GetDNSHostname(
    const RDMRequest *request,
    const NetworkManagerInterface *network_manager,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  const string hostname = network_manager->GetHostname();
  if (hostname.empty() || hostname.length() > MAX_RDM_HOSTNAME_LENGTH) {
    // Hostname outside of the allowed parameters for RDM, return an error
    return NackWithReason(request, NR_HARDWARE_FAULT);
  } else {
    return GetString(request,
                     hostname,
                     queued_message_count,
                     MAX_RDM_HOSTNAME_LENGTH);
  }
}


RDMResponse *ResponderHelper::GetDNSDomainName(
    const RDMRequest *request,
    const NetworkManagerInterface *network_manager,
    uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }

  string domain_name = network_manager->GetDomainName();
  if (domain_name.length() > MAX_RDM_DOMAIN_NAME_LENGTH) {
    // Domain name outside of the allowed parameters for RDM, return an error
    return NackWithReason(request, NR_HARDWARE_FAULT);
  } else {
    return GetString(request,
                     domain_name,
                     queued_message_count,
                     MAX_RDM_DOMAIN_NAME_LENGTH);
  }
}

RDMResponse *ResponderHelper::GetDNSNameServer(
    const RDMRequest *request,
    const NetworkManagerInterface *network_manager,
    uint8_t queued_message_count) {
  uint8_t name_server_number;
  if (!ResponderHelper::ExtractUInt8(request, &name_server_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  vector<IPV4Address> name_servers;
  if (!network_manager->GetNameServers(&name_servers)) {
    return NackWithReason(request, NR_HARDWARE_FAULT);
  }

  if ((name_server_number >= name_servers.size()) ||
      (name_server_number > DNS_NAME_SERVER_MAX_INDEX)) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  PACK(
  struct name_server_s {
    uint8_t index;
    uint32_t address;
  });
  STATIC_ASSERT(sizeof(name_server_s) == 5);

  struct name_server_s name_server;
  name_server.index = name_server_number;
  // s_addr is already in network byte order, so doesn't need converting
  name_server.address = name_servers.at(name_server_number).AsInt();

  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&name_server),
    sizeof(name_server),
    RDM_ACK,
    queued_message_count);
}

RDMResponse *ResponderHelper::GetParamDescription(
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
  PACK(
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
  });
  STATIC_ASSERT(sizeof(parameter_description_s) == 52);

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

  size_t str_len = min(description.size(),
                       sizeof(param_description.description));
  strncpy(param_description.description, description.c_str(), str_len);

  unsigned int param_data_size = (
      sizeof(param_description) -
      sizeof(param_description.description) + str_len);

  return GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&param_description),
      param_data_size,
      RDM_ACK,
      queued_message_count);
}

RDMResponse *ResponderHelper::GetASCIIParamDescription(
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

RDMResponse *ResponderHelper::GetBitFieldParamDescription(
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
 * Handle a request that returns an IPv4 address
 */
RDMResponse *ResponderHelper::GetIPV4Address(
    const RDMRequest *request,
    const IPV4Address &value,
    uint8_t queued_message_count) {
  return GetUInt32Value(request,
                        // Flip it back because s_addr is in network byte order
                        // already
                        NetworkToHost(value.AsInt()),
                        queued_message_count);
}

/**
 * @brief Handle a request that returns a string
 * @note this truncates the string to max_length
 */
RDMResponse *ResponderHelper::GetString(
    const RDMRequest *request,
    const string &value,
    uint8_t queued_message_count,
    uint8_t max_length) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  string sanitised_value = value.substr(
      0,
      min(static_cast<uint8_t>(value.length()), max_length));
  return GetResponseFromData(
      request,
      reinterpret_cast<const uint8_t*>(sanitised_value.data()),
      sanitised_value.size(),
      RDM_ACK,
      queued_message_count);
}

RDMResponse *ResponderHelper::EmptyGetResponse(
    const RDMRequest *request,
    uint8_t queued_message_count) {
  return GetResponseFromData(request,
                             NULL,
                             0,
                             RDM_ACK,
                             queued_message_count);
}

RDMResponse *ResponderHelper::EmptySetResponse(
    const RDMRequest *request,
    uint8_t queued_message_count) {
  return GetResponseFromData(request,
                             NULL,
                             0,
                             RDM_ACK,
                             queued_message_count);
}

RDMResponse *ResponderHelper::SetString(
    const RDMRequest *request,
    string *value,
    uint8_t queued_message_count,
    uint8_t max_length) {
  if (request->ParamDataSize() > max_length) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  const string new_label(reinterpret_cast<const char*>(request->ParamData()),
                         request->ParamDataSize());
  *value = new_label;
  return EmptySetResponse(request, queued_message_count);
}

RDMResponse *ResponderHelper::GetBoolValue(const RDMRequest *request,
                                                 bool value,
                                                 uint8_t queued_message_count) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  uint8_t param = value ? 1 : 0;

  return GetResponseFromData(request, &param, sizeof(param),
                             RDM_ACK, queued_message_count);
}

RDMResponse *ResponderHelper::SetBoolValue(const RDMRequest *request,
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
static RDMResponse *GenericGetIntValue(const RDMRequest *request,
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

RDMResponse *ResponderHelper::GetUInt8Value(
    const RDMRequest *request,
    uint8_t value,
    uint8_t queued_message_count) {
  return GenericGetIntValue(request, value, queued_message_count);
}

RDMResponse *ResponderHelper::GetUInt16Value(
    const RDMRequest *request,
    uint16_t value,
    uint8_t queued_message_count) {
  return GenericGetIntValue(request, value, queued_message_count);
}

RDMResponse *ResponderHelper::GetUInt32Value(
    const RDMRequest *request,
    uint32_t value,
    uint8_t queued_message_count) {
  return GenericGetIntValue(request, value, queued_message_count);
}

template<typename T>
static RDMResponse *GenericSetIntValue(const RDMRequest *request,
                                             T *value,
                                             uint8_t queued_message_count = 0) {
  if (!GenericExtractValue(request, value)) {
    return NackWithReason(request, NR_FORMAT_ERROR, queued_message_count);
  }
  return ResponderHelper::EmptySetResponse(request, queued_message_count);
}

RDMResponse *ResponderHelper::SetUInt8Value(
    const RDMRequest *request,
    uint8_t *value,
    uint8_t queued_message_count) {
  return GenericSetIntValue(request, value, queued_message_count);
}

RDMResponse *ResponderHelper::SetUInt16Value(
    const RDMRequest *request,
    uint16_t *value,
    uint8_t queued_message_count) {
  return GenericSetIntValue(request, value, queued_message_count);
}

RDMResponse *ResponderHelper::SetUInt32Value(
    const RDMRequest *request,
    uint32_t *value,
    uint8_t queued_message_count) {
  return GenericSetIntValue(request, value, queued_message_count);
}


bool ResponderHelper::FindInterface(
    const NetworkManagerInterface *network_manager,
    Interface *interface, uint32_t index) {
  if (!IsInterfaceIndexValid(index)) {
    // Invalid index
    return false;
  }

  InterfacePicker::Options options;
  options.specific_only = true;
  return network_manager->GetInterfacePicker()->ChooseInterface(
      interface, index, options);
}

bool ResponderHelper::IsInterfaceIndexValid(uint32_t index) {
  return (index >= MIN_RDM_INTERFACE_INDEX &&
          index <= MAX_RDM_INTERFACE_INDEX);
}

bool ResponderHelper::IsInterfaceIndexValidInterface(Interface interface) {
  return IsInterfaceIndexValid(interface.index);
}
}  // namespace rdm
}  // namespace ola
