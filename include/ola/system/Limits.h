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
 * Limits.h
 * Functions that deal with system limits (rlimits)
 * Copyright (C) 2014 Simon Newton
 */

#ifndef INCLUDE_OLA_SYSTEM_LIMITS_H_
#define INCLUDE_OLA_SYSTEM_LIMITS_H_

#ifndef _WIN32
#include <sys/resource.h>

namespace ola {
namespace system {

/**
 * @brief Get the current values for an rlimit.
 * @param resource the rlimit to fetch.
 * @param lim The rlimit struct to populate.
 * @returns true if the call succeeded, false otherwise.
 */
bool GetRLimit(int resource, struct rlimit *lim);

/**
 * @brief Set the values for an rlimit.
 * @param resource the rlimit to set.
 * @param lim The rlimit struct with the values to set.
 * @returns true if the call succeeded, false otherwise.
 */
bool SetRLimit(int resource, const struct rlimit &lim);
}  // namespace system
}  // namespace ola
#endif  // _WIN32
#endif  // INCLUDE_OLA_SYSTEM_LIMITS_H_
