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
 * SystemUtils.h
 * System Util functions.
 * Copyright (C) 2013 Peter Newman
 */

#ifndef INCLUDE_OLA_SYSTEM_SYSTEMUTILS_H_
#define INCLUDE_OLA_SYSTEM_SYSTEMUTILS_H_

#include <stdint.h>
#include <stdlib.h>

namespace ola {
namespace system {

static const uint8_t NUMBER_LOAD_AVERAGES = 3;

typedef enum {
  LOAD_AVERAGE_1_MIN = 0,
  LOAD_AVERAGE_5_MINS = 1,
  LOAD_AVERAGE_15_MINS = 2
} load_averages;

/**
 * @brief Get the system load average
 * @param[in] average the load average to fetch
 * @param[out] value a pointer to where the value will be stored
 * @returns true if the requested load average was fetched, false otherwise
 */
bool LoadAverage(load_averages average, double *value);
}  // namespace system
}  // namespace ola
#endif  // INCLUDE_OLA_SYSTEM_SYSTEMUTILS_H_
