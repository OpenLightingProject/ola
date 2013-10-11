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
 * ResponderSensor.h
 * Manages a sensor for a RDM responder.
 * Copyright (C) 2013 Peter Newman
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file ResponderSensor.h
 * @brief Holds the information about a sensor.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RESPONDERSENSOR_H_
#define INCLUDE_OLA_RDM_RESPONDERSENSOR_H_

#include <stdint.h>
#include <algorithm>
#include <string>
#include <vector>

#include "ola/rdm/RDMEnums.h"

namespace ola {
namespace rdm {

using std::string;

/**
 * @brief Holds information about a single sensor.
 */
class Sensor {
  public:
    Sensor(ola::rdm::rdm_sensor_type type,
           ola::rdm::rdm_pid_unit unit,
           ola::rdm::rdm_pid_prefix prefix,
           const string &description = "",
           bool recorded_value_support = true,
           bool recorded_range_support = true,
           int16_t range_min = SENSOR_DEFINITION_RANGE_MIN_UNDEFINED,
           int16_t range_max = SENSOR_DEFINITION_RANGE_MAX_UNDEFINED,
           int16_t normal_min = SENSOR_DEFINITION_NORMAL_MIN_UNDEFINED,
           int16_t normal_max = SENSOR_DEFINITION_NORMAL_MAX_UNDEFINED)
        : m_type(type),
          m_unit(unit),
          m_prefix(prefix),
          m_description(description),
          m_recorded_value_support(recorded_value_support),
          m_recorded_range_support(recorded_range_support),
          m_range_min(range_min),
          m_range_max(range_max),
          m_normal_min(normal_min),
          m_normal_max(normal_max),
          m_recorded(0) {
    };

    rdm_sensor_type Type() const { return m_type; }
    rdm_pid_unit Unit() const { return m_unit; }
    rdm_pid_prefix Prefix() const { return m_prefix; }
    int16_t RangeMin() const { return m_range_min; }
    int16_t RangeMax() const { return m_range_max; }
    int16_t NormalMin() const { return m_normal_min; }
    int16_t NormalMax() const { return m_normal_max; }
    string Description() const { return m_description; }
    virtual int16_t PollSensor() = 0;

    int16_t Lowest() {
      if (m_recorded_range_support) {
        return m_lowest;
      } else {
        return SENSOR_RECORDED_RANGE_UNSUPPORTED;
      }
    }

    int16_t Highest() {
      if (m_recorded_range_support) {
        return m_highest;
      } else {
        return SENSOR_RECORDED_RANGE_UNSUPPORTED;
      }
    }

    int16_t Recorded() {
      if (m_recorded_value_support) {
        return m_recorded;
      } else {
        return SENSOR_RECORDED_UNSUPPORTED;
      }
    }

    /**
     * Generate a new value.
     */
    int16_t FetchValue() {
      int16_t value = PollSensor();
      m_lowest = std::min(value, m_lowest);
      m_highest = std::max(value, m_highest);
      return value;
    }

    /**
     * Generate a new value and record it.
     */
    void Record() {
      uint16_t value = FetchValue();
      m_recorded = value;
    }

    /**
     * Reset a sensor.
     */
    int16_t Reset() {
      int16_t value = PollSensor();
      m_lowest = value;
      m_highest = value;
      m_recorded = value;
      return value;
    }

    uint8_t RecordedSupportBitMask() {
      uint8_t bit_mask = 0;
      if (m_recorded_value_support) {
        bit_mask |= SENSOR_RECORDED_VALUE;
      }
      if (m_recorded_range_support) {
        bit_mask |= SENSOR_RECORDED_RANGE_VALUES;
      }
      return bit_mask;
    }

  protected:
    const ola::rdm::rdm_sensor_type m_type;
    const ola::rdm::rdm_pid_unit m_unit;
    const ola::rdm::rdm_pid_prefix m_prefix;
    const string m_description;
    const bool m_recorded_value_support;
    const bool m_recorded_range_support;
    const int16_t m_range_min;
    const int16_t m_range_max;
    const int16_t m_normal_min;
    const int16_t m_normal_max;
    int16_t m_lowest;
    int16_t m_highest;
    int16_t m_recorded;
};

typedef std::vector<ola::rdm::Sensor*> Sensors;
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERSENSOR_H_
