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
 * SensorResponder.cpp
 * Copyright (C) 2013 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/stl/STLUtils.h"
#include "ola/math/Random.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/SensorResponder.h"

namespace ola {
namespace rdm {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

SensorResponder::RDMOps *SensorResponder::RDMOps::instance = NULL;

const ResponderOps<SensorResponder>::ParamHandler
    SensorResponder::PARAM_HANDLERS[] = {
  { PID_DEVICE_INFO,
    &SensorResponder::GetDeviceInfo,
    NULL},
  { PID_PRODUCT_DETAIL_ID_LIST,
    &SensorResponder::GetProductDetailList,
    NULL},
  { PID_DEVICE_MODEL_DESCRIPTION,
    &SensorResponder::GetDeviceModelDescription,
    NULL},
  { PID_MANUFACTURER_LABEL,
    &SensorResponder::GetManufacturerLabel,
    NULL},
  { PID_DEVICE_LABEL,
    &SensorResponder::GetDeviceLabel,
    NULL},
  { PID_SOFTWARE_VERSION_LABEL,
    &SensorResponder::GetSoftwareVersionLabel,
    NULL},
  { PID_SENSOR_DEFINITION,
    &SensorResponder::GetSensorDefinition,
    NULL},
  { PID_SENSOR_VALUE,
    &SensorResponder::GetSensorValue,
    &SensorResponder::SetSensorValue},
  { PID_RECORD_SENSORS,
    NULL,
    &SensorResponder::RecordSensor},
  { PID_IDENTIFY_DEVICE,
    &SensorResponder::GetIdentify,
    &SensorResponder::SetIdentify},
  { 0, NULL, NULL},
};


/**
 * A class which represents a sensor.
 */
class FakeSensor {
  public:
    FakeSensor(ola::rdm::rdm_sensor_type type,
               ola::rdm::rdm_pid_unit unit,
               ola::rdm::rdm_pid_prefix prefix,
               int16_t range_min,
               int16_t range_max,
               int16_t normal_min,
               int16_t normal_max,
               const string &description)
        : type(type),
          unit(unit),
          prefix(prefix),
          range_min(range_min),
          range_max(range_max),
          normal_min(normal_min),
          normal_max(normal_max),
          description(description),
          recorded(0) {
      // set high / low to something
      highest = lowest = range_min + (range_max - range_min) / 2;
    }

    int16_t GenerateValue();
    void Record();
    int16_t Reset();

    const ola::rdm::rdm_sensor_type type;
    const ola::rdm::rdm_pid_unit unit;
    const ola::rdm::rdm_pid_prefix prefix;
    const int16_t range_min;
    const int16_t range_max;
    const int16_t normal_min;
    const int16_t normal_max;
    const string description;

    int16_t highest;
    int16_t lowest;
    int16_t recorded;
};


/**
 * Generate a Sensor value
 */
int16_t FakeSensor::GenerateValue() {
  int16_t value = ola::math::Random(range_min, range_max);
  lowest = std::min(value, lowest);
  highest = std::max(value, highest);
  return value;
}


/**
 * Generate a new value and record it.
 */
void FakeSensor::Record() {
  uint16_t value = GenerateValue();
  recorded = value;
}

/**
 * Reset a sensor.
 */
int16_t FakeSensor::Reset() {
  int16_t value = GenerateValue();
  lowest = value;
  highest = value;
  recorded = value;
  return value;
}


/**
 * New SensorResponder
 */
SensorResponder::SensorResponder(const UID &uid)
    : m_uid(uid),
      m_identify_mode(false) {
  m_sensors.push_back(new FakeSensor(
        SENSOR_TEMPERATURE, UNITS_CENTIGRADE, PREFIX_NONE,
        0, 100, 10, 20, "Fake Temperature"));
  m_sensors.push_back(new FakeSensor(
        SENSOR_VOLTAGE, UNITS_VOLTS_DC, PREFIX_DECI,
        110, 140, 119, 125, "Fake Voltage"));
  m_sensors.push_back(new FakeSensor(
        SENSOR_ITEMS, UNITS_NONE, PREFIX_KILO,
        0, 100, 0, 1, "Fake Beta Particle Counter"));
}


SensorResponder::~SensorResponder() {
  STLDeleteElements(&m_sensors);
}

/*
 * Handle an RDM Request
 */
void SensorResponder::SendRDMRequest(const RDMRequest *request,
                                          RDMCallback *callback) {
  RDMOps::Instance()->HandleRDMRequest(this, m_uid, ROOT_RDM_DEVICE, request,
                                       callback);
}

const RDMResponse *SensorResponder::GetDeviceInfo(
    const RDMRequest *request) {
  return ResponderHelper::GetDeviceInfo(
      request, OLA_SENSOR_ONLY_MODEL, PRODUCT_CATEGORY_TEST,
      1, 0, 1, 1, ZERO_FOOTPRINT_DMX_ADDRESS, 0, m_sensors.size());
}

const RDMResponse *SensorResponder::GetProductDetailList(
    const RDMRequest *request) {
  // Shortcut for only one item in the vector
  return ResponderHelper::GetProductDetailList(
      request, vector<rdm_product_detail>(1, PRODUCT_DETAIL_TEST));
}

const RDMResponse *SensorResponder::GetIdentify(
    const RDMRequest *request) {
  return ResponderHelper::GetBoolValue(request, m_identify_mode);
}

const RDMResponse *SensorResponder::SetIdentify(
    const RDMRequest *request) {
  bool old_value = m_identify_mode;
  const RDMResponse *response = ResponderHelper::SetBoolValue(
      request, &m_identify_mode);
  if (m_identify_mode != old_value) {
    OLA_INFO << "Sensor Device " << m_uid << ", identify mode "
             << (m_identify_mode ? "on" : "off");
  }
  return response;
}

const RDMResponse *SensorResponder::GetDeviceModelDescription(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "OLA Sensor Device");
}

const RDMResponse *SensorResponder::GetManufacturerLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, OLA_MANUFACTURER_LABEL);
}

const RDMResponse *SensorResponder::GetDeviceLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, "Sensor Device");
}

const RDMResponse *SensorResponder::GetSoftwareVersionLabel(
    const RDMRequest *request) {
  return ResponderHelper::GetString(request, string("OLA Version ") + VERSION);
}

/**
 * PID_SENSOR_DEFINITION
 */
const RDMResponse *SensorResponder::GetSensorDefinition(
    const RDMRequest *request) {
  uint8_t sensor_number;
  if (!ResponderHelper::ExtractUInt8(request, &sensor_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (sensor_number >= m_sensors.size()) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

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
    char description[32];
  } __attribute__((packed));

  FakeSensor *sensor = m_sensors[sensor_number];
  struct sensor_definition_s sensor_definition;
  sensor_definition.sensor =  sensor_number;
  sensor_definition.type = sensor->type;
  sensor_definition.unit = sensor->unit;
  sensor_definition.prefix = sensor->prefix;
  sensor_definition.range_min = HostToNetwork(sensor->range_min);
  sensor_definition.range_max = HostToNetwork(sensor->range_max);
  sensor_definition.normal_min = HostToNetwork(sensor->normal_min);
  sensor_definition.normal_max = HostToNetwork(sensor->normal_max);
  sensor_definition.recorded_support = (
      SENSOR_RECORDED_VALUE | SENSOR_RECORDED_RANGE_VALUES);
  strncpy(sensor_definition.description, sensor->description.c_str(),
          sizeof(sensor_definition.description));

  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&sensor_definition),
    sizeof(sensor_definition));
}

/**
 * PID_SENSOR_VALUE
 */
const RDMResponse *SensorResponder::GetSensorValue(const RDMRequest *request) {
  uint8_t sensor_number;
  if (!ResponderHelper::ExtractUInt8(request, &sensor_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (sensor_number >= m_sensors.size()) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  FakeSensor *sensor = m_sensors[sensor_number];
  struct sensor_value_s sensor_value = {
    sensor_number,
    HostToNetwork(sensor->GenerateValue()),
    HostToNetwork(sensor->lowest),
    HostToNetwork(sensor->highest),
    HostToNetwork(sensor->recorded),
  };

  return GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&sensor_value),
    sizeof(sensor_value));
}

const RDMResponse *SensorResponder::SetSensorValue(const RDMRequest *request) {
  uint8_t sensor_number;
  if (!ResponderHelper::ExtractUInt8(request, &sensor_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  int16_t value = 0;
  if (sensor_number == ALL_SENSORS) {
    FakeSensors::const_iterator iter = m_sensors.begin();
    for (; iter != m_sensors.end(); ++iter) {
      value = (*iter)->Reset();
    }
  } else if (sensor_number < m_sensors.size()) {
    FakeSensor *sensor = m_sensors[sensor_number];
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
 * PID_RECORD_SENSORS
 */
const RDMResponse *SensorResponder::RecordSensor(const RDMRequest *request) {
  uint8_t sensor_number;
  if (!ResponderHelper::ExtractUInt8(request, &sensor_number)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  if (sensor_number == ALL_SENSORS) {
    FakeSensors::const_iterator iter = m_sensors.begin();
    for (; iter != m_sensors.end(); ++iter) {
      (*iter)->Record();
    }
  } else if (sensor_number < m_sensors.size()) {
    FakeSensor *sensor = m_sensors[sensor_number];
    sensor->Record();
  } else {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  }

  return GetResponseFromData(request, NULL, 0);
}
}  // namespace rdm
}  // namespace ola
