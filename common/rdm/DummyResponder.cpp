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
 * DummyResponder.cpp
 * Copyright (C) 2005 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <iostream>
#include <string>
#include <vector>
#include "ola/base/Array.h"
#include "ola/Clock.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/DummyResponder.h"
#include "common/rdm/NetworkManager.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/ResponderLoadSensor.h"
#include "ola/rdm/ResponderSensor.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

DummyResponder::RDMOps *DummyResponder::RDMOps::instance = NULL;

const DummyResponder::Personalities *
    DummyResponder::Personalities::Instance() {
  if (!instance) {
    SlotDataCollection::SlotDataList p2_slot_data;
    p2_slot_data.push_back(SlotData::PrimarySlot(SD_INTENSITY, 0));
    p2_slot_data.push_back(SlotData::SecondarySlot(ST_SEC_FINE, 0, 0));
    p2_slot_data.push_back(SlotData::PrimarySlot(SD_PAN, 127));
    p2_slot_data.push_back(SlotData::PrimarySlot(SD_TILT, 127));
    p2_slot_data.push_back(SlotData::PrimarySlot(SD_UNDEFINED, 0, "Foo"));

    PersonalityList personalities;
    personalities.push_back(Personality(0, "Personality 1"));
    personalities.push_back(Personality(5,
                                        "Personality 2",
                                        SlotDataCollection(p2_slot_data)));
    personalities.push_back(Personality(10, "Personality 3"));
    personalities.push_back(Personality(20, "Personality 4"));
    instance = new Personalities(personalities);
  }
  return instance;
}

DummyResponder::Personalities * DummyResponder::Personalities::instance = NULL;

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
  { PID_SLOT_INFO,
    &DummyResponder::GetSlotInfo,
    NULL},
  { PID_SLOT_DESCRIPTION,
    &DummyResponder::GetSlotDescription,
    NULL},
  { PID_DEFAULT_SLOT_VALUE,
    &DummyResponder::GetSlotDefaultValues,
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
#ifdef HAVE_GETLOADAVG
  { PID_SENSOR_DEFINITION,
    &DummyResponder::GetSensorDefinition,
    NULL},
  { PID_SENSOR_VALUE,
    &DummyResponder::GetSensorValue,
    &DummyResponder::SetSensorValue},
  { PID_RECORD_SENSORS,
    NULL,
    &DummyResponder::RecordSensor},
#endif  // HAVE_GETLOADAVG
  { PID_LIST_INTERFACES,
    &DummyResponder::GetListInterfaces,
    NULL},
  { PID_INTERFACE_LABEL,
    &DummyResponder::GetInterfaceLabel,
    NULL},
  { PID_INTERFACE_HARDWARE_ADDRESS_TYPE1,
    &DummyResponder::GetInterfaceHardwareAddressType1,
    NULL},
  { PID_IPV4_CURRENT_ADDRESS,
    &DummyResponder::GetIPV4CurrentAddress,
    NULL},
  { PID_IPV4_DEFAULT_ROUTE,
    &DummyResponder::GetIPV4DefaultRoute,
    NULL},
  { PID_DNS_HOSTNAME,
    &DummyResponder::GetDNSHostname,
    NULL},
  { PID_DNS_DOMAIN_NAME,
    &DummyResponder::GetDNSDomainName,
    NULL},
  { PID_DNS_NAME_SERVER,
    &DummyResponder::GetDNSNameServer,
    NULL},
  { OLA_MANUFACTURER_PID_CODE_VERSION,
    &DummyResponder::GetOlaCodeVersion,
    NULL},
  { 0, NULL, NULL},
};

DummyResponder::DummyResponder(const UID &uid)
    : m_uid(uid),
      m_start_address(1),
      m_identify_mode(0),
      m_lamp_strikes(0),
      m_personality_manager(Personalities::Instance()) {
  // default to a personality with a non-0 footprint.
  m_personality_manager.SetActivePersonality(DEFAULT_PERSONALITY);

#ifdef HAVE_GETLOADAVG
  m_sensors.push_back(new LoadSensor(ola::system::LOAD_AVERAGE_1_MIN,
                                     "Load Average 1 minute"));
  m_sensors.push_back(new LoadSensor(ola::system::LOAD_AVERAGE_5_MINS,
                                     "Load Average 5 minutes"));
  m_sensors.push_back(new LoadSensor(ola::system::LOAD_AVERAGE_15_MINS,
                                     "Load Average 15 minutes"));
#endif  // HAVE_GETLOADAVG

  m_network_manager.reset(new NetworkManager());
}

DummyResponder::~DummyResponder() {
  STLDeleteElements(&m_sensors);
}

/*
 * Handle an RDM Request
 */
void DummyResponder::SendRDMRequest(RDMRequest *request,
                                    RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ola::rdm::ROOT_RDM_DEVICE,
                                       request, callback);
}

RDMResponse *DummyResponder::GetParamDescription(
    const RDMRequest *request) {
  // Check that it's OLA_MANUFACTURER_PID_CODE_VERSION being requested
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

RDMResponse *DummyResponder::GetDeviceInfo(const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_DUMMY_DEVICE_MODEL,
      PRODUCT_CATEGORY_OTHER, 3,
      &m_personality_manager,
      m_start_address,
      0, m_sensors.size());
}

/**
 * Reset to factory defaults
 */
RDMResponse *DummyResponder::GetFactoryDefaults(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  uint8_t using_defaults = (
      m_start_address == 1 &&
      m_personality_manager.ActivePersonalityNumber() == DEFAULT_PERSONALITY &&
      m_identify_mode == false);
  return GetResponseFromData(request, &using_defaults, sizeof(using_defaults));
}

RDMResponse *DummyResponder::SetFactoryDefaults(
    const RDMRequest *request) {
  if (request->ParamDataSize()) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  m_start_address = 1;
  m_personality_manager.SetActivePersonality(DEFAULT_PERSONALITY);
  m_identify_mode = 0;

  return ResponderHelper::EmptySetResponse(request);
}

RDMResponse *DummyResponder::GetProductDetailList(
    const RDMRequest *request) {
  vector<rdm_product_detail> product_details;
  product_details.push_back(PRODUCT_DETAIL_TEST);
  product_details.push_back(PRODUCT_DETAIL_OTHER);
  return ResponderHelper::GetProductDetailList(request, product_details);
}

RDMResponse *DummyResponder::GetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonality(request, &m_personality_manager);
}

RDMResponse *DummyResponder::SetPersonality(
    const RDMRequest *request) {
  return ResponderHelper::SetPersonality(request, &m_personality_manager,
                                         m_start_address);
}

RDMResponse *DummyResponder::GetPersonalityDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetPersonalityDescription(
      request, &m_personality_manager);
}

RDMResponse *DummyResponder::GetSlotInfo(const RDMRequest *request) {
  return ResponderHelper::GetSlotInfo(request, &m_personality_manager);
}

RDMResponse *DummyResponder::GetSlotDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetSlotDescription(request, &m_personality_manager);
}

RDMResponse *DummyResponder::GetSlotDefaultValues(
    const RDMRequest *request) {
  return ResponderHelper::GetSlotDefaultValues(request, &m_personality_manager);
}

RDMResponse *DummyResponder::GetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::GetDmxAddress(request, &m_personality_manager,
                                        m_start_address);
}

RDMResponse *DummyResponder::SetDmxStartAddress(
    const RDMRequest *request) {
  return ResponderHelper::SetDmxAddress(request, &m_personality_manager,
                                        &m_start_address);
}

RDMResponse *DummyResponder::GetLampStrikes(const RDMRequest *request) {
  return ResponderHelper::GetUInt32Value(request, m_lamp_strikes);
}

RDMResponse *DummyResponder::SetLampStrikes(const RDMRequest *request) {
  return ResponderHelper::SetUInt32Value(request, &m_lamp_strikes);
}

RDMResponse *DummyResponder::GetIdentify(const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

RDMResponse *DummyResponder::SetIdentify(const RDMRequest *request) {
  bool old_value = m_identify_mode;
  RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "Dummy device, identify mode "
             << (m_identify_mode ? "on" : "off");
  }
  return response;
}

RDMResponse *DummyResponder::GetRealTimeClock(const RDMRequest *request) {
  return ResponderHelper::GetRealTimeClock(request);
}

RDMResponse *DummyResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

RDMResponse *DummyResponder::GetDeviceLabel(const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy RDM Device");
}

RDMResponse *DummyResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy Model");
}

RDMResponse *DummyResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Dummy Software Version");
}

/**
 * PID_SENSOR_DEFINITION
 */
RDMResponse *DummyResponder::GetSensorDefinition(
    const RDMRequest *request) {
  return ResponderHelper::GetSensorDefinition(request, m_sensors);
}

/**
 * PID_SENSOR_VALUE
 */
RDMResponse *DummyResponder::GetSensorValue(const RDMRequest *request) {
  return ResponderHelper::GetSensorValue(request, m_sensors);
}

RDMResponse *DummyResponder::SetSensorValue(const RDMRequest *request) {
  return ResponderHelper::SetSensorValue(request, m_sensors);
}

/**
 * PID_RECORD_SENSORS
 */
RDMResponse *DummyResponder::RecordSensor(const RDMRequest *request) {
  return ResponderHelper::RecordSensor(request, m_sensors);
}

/**
 * E1.37-2 PIDs
 */
RDMResponse *DummyResponder::GetListInterfaces(
    const RDMRequest *request) {
  return ResponderHelper::GetListInterfaces(request,
                                            m_network_manager.get());
}

RDMResponse *DummyResponder::GetInterfaceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetInterfaceLabel(request,
                                            m_network_manager.get());
}

RDMResponse *DummyResponder::GetInterfaceHardwareAddressType1(
    const RDMRequest *request) {
  return ResponderHelper::GetInterfaceHardwareAddressType1(
      request,
      m_network_manager.get());
}

RDMResponse *DummyResponder::GetIPV4CurrentAddress(
    const RDMRequest *request) {
  return ResponderHelper::GetIPV4CurrentAddress(request,
                                                m_network_manager.get());
}

RDMResponse *DummyResponder::GetIPV4DefaultRoute(
    const RDMRequest *request) {
  return ResponderHelper::GetIPV4DefaultRoute(request,
                                              m_network_manager.get());
}

RDMResponse *DummyResponder::GetDNSHostname(
    const RDMRequest *request) {
  return ResponderHelper::GetDNSHostname(request,
                                         m_network_manager.get());
}

RDMResponse *DummyResponder::GetDNSDomainName(
    const RDMRequest *request) {
  return ResponderHelper::GetDNSDomainName(request,
                                           m_network_manager.get());
}

RDMResponse *DummyResponder::GetDNSNameServer(
    const RDMRequest *request) {
  return ResponderHelper::GetDNSNameServer(request,
                                           m_network_manager.get());
}

RDMResponse *DummyResponder::GetOlaCodeVersion(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, VERSION);
}
}  // namespace rdm
}  // namespace ola
