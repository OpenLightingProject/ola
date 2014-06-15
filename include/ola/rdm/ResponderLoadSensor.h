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

#include <ola/rdm/ResponderSensor.h>
#include <ola/system/SystemUtils.h>

#include <string>

namespace ola {
namespace rdm {
/**
 * A class which represents a load sensor.
 */
class LoadSensor: public Sensor {
 public:
  LoadSensor(const ola::system::load_averages load_average,
             const std::string &description)
      : Sensor(SENSOR_OTHER,
               UNITS_NONE,
               PREFIX_CENTI,
               description,
               SensorOptions(true,
                             true,
                             0,
                             SENSOR_DEFINITION_RANGE_MAX_UNDEFINED,
                             0,
                             SENSOR_DEFINITION_NORMAL_MAX_UNDEFINED)),
        m_load_average(load_average) {
    // set high / low to something
    Reset();
    // Force recorded back to zero
    m_recorded = 0;
  }

  static const int16_t LOAD_SENSOR_ERROR_VALUE = 0;

 protected:
  int16_t PollSensor();

 private:
  ola::system::load_averages m_load_average;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERLOADSENSOR_H_
