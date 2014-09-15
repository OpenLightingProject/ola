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

#include "ola/Logging.h"
#include "ola/rdm/ResponderLoadSensor.h"
#include "ola/system/SystemUtils.h"

namespace ola {
namespace rdm {
/**
 * Fetch a Sensor value
 */
int16_t LoadSensor::PollSensor() {
  double average;
  if (!ola::system::LoadAverage(m_load_average, &average)) {
    return LOAD_SENSOR_ERROR_VALUE;
  } else {
    return static_cast<int16_t>(average * 100);
  }
}
}  // namespace rdm
}  // namespace ola
