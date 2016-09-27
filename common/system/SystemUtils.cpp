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
 * SystemUtils.cpp
 * System Helper methods.
 * Copyright (C) 2013 Peter Newman
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include "ola/Logging.h"
#include "ola/system/SystemUtils.h"

namespace ola {
namespace system {

bool LoadAverage(load_averages average, double *value) {
#ifdef HAVE_GETLOADAVG
  if (average >= NUMBER_LOAD_AVERAGES) {
    return false;
  }
  double averages[NUMBER_LOAD_AVERAGES];
  uint8_t returned;
  returned = getloadavg(averages, NUMBER_LOAD_AVERAGES);
  if (returned != NUMBER_LOAD_AVERAGES) {
    OLA_WARN << "getloadavg only returned " << static_cast<int>(returned)
        << " values, expecting " << static_cast<int>(NUMBER_LOAD_AVERAGES)
        << " values";
    return false;
  } else {
    *value = averages[average];
    return true;
  }
#else
  // No getloadavg, do something else if Windows?
  OLA_WARN << "getloadavg not supported, can't fetch value";
  (void) average;
  *value = 0;
  return false;
#endif  // HAVE_GETLOADAVG
}

}  // namespace system
}  // namespace ola
