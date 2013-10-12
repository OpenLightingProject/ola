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
 * ResponderLoadSensor.h
 * Copyright (C) 2013 Peter Newman
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file ResponderLoadSensor.h
 * @brief Holds the information about a load sensor.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RESPONDERLOADSENSOR_H_
#define INCLUDE_OLA_RDM_RESPONDERLOADSENSOR_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstdlib>
#include <string>

namespace ola {
namespace rdm {
/**
 * A class which represents a load sensor.
 */
class LoadSensor: public Sensor {
  public:
    LoadSensor(const uint8_t load_average, const string &description = "")
        : Sensor(SENSOR_OTHER,
                 UNITS_NONE,
                 PREFIX_CENTI,
                 description,
                 true,
                 true,
                 0,
                 SENSOR_DEFINITION_RANGE_MAX_UNDEFINED,
                 0,
                 SENSOR_DEFINITION_NORMAL_MAX_UNDEFINED),
          m_load_average(load_average) {
      // set high / low to something
      Reset();
      // Force recorded back to zero
      m_recorded = 0;
    }

    int16_t PollSensor();

    static const int16_t LOAD_SENSOR_NUM_AVERAGES = 3;
    static const int16_t LOAD_SENSOR_ERROR_VALUE = 0;

  private:
    uint8_t m_load_average;
};


/**
 * Fetch a Sensor value
 */
int16_t LoadSensor::PollSensor() {
#ifdef HAVE_GETLOADAVG
  if (m_load_average >= LOAD_SENSOR_NUM_AVERAGES) {
    return LOAD_SENSOR_ERROR_VALUE;
  }
  double averages[LOAD_SENSOR_NUM_AVERAGES];
  uint8_t returned;
  returned = getloadavg(averages, LOAD_SENSOR_NUM_AVERAGES);
  if (returned != LOAD_SENSOR_NUM_AVERAGES) {
    OLA_WARN << "getloadavg only returned " << static_cast<int>(returned) <<
        " values, expecting " << static_cast<int>(LOAD_SENSOR_NUM_AVERAGES) <<
        " values";
    return LOAD_SENSOR_ERROR_VALUE;
  } else {
    return static_cast<int16_t>(averages[m_load_average]*100);
  }
#else
  // No getloadavg, do something else if Windows?
  OLA_WARN << "getloadavg not supported, returning default value";
  return LOAD_SENSOR_ERROR_VALUE;
#endif
}
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERLOADSENSOR_H_
