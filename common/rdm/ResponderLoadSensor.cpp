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
 * ResponderLoadSensor.cpp
 * Copyright (C) 2013 Peter Newman
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ola/Logging.h"
#include "ola/rdm/ResponderLoadSensor.h"

namespace ola {
namespace rdm {
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
    OLA_WARN << "getloadavg only returned " << static_cast<int>(returned)
        << " values, expecting " << static_cast<int>(LOAD_SENSOR_NUM_AVERAGES)
        << " values";
    return LOAD_SENSOR_ERROR_VALUE;
  } else {
    return static_cast<int16_t>(averages[m_load_average] * 100);
  }
#else
  // No getloadavg, do something else if Windows?
  OLA_WARN << "getloadavg not supported, returning default value";
  return LOAD_SENSOR_ERROR_VALUE;
#endif
}
}  // namespace rdm
}  // namespace ola
