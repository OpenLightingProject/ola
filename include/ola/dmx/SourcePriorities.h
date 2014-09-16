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
 * SourcePriorities.h
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @file SourcePriorities.h
 * @brief The constants for DMX source priorities.
 */

#ifndef INCLUDE_OLA_DMX_SOURCEPRIORITIES_H_
#define INCLUDE_OLA_DMX_SOURCEPRIORITIES_H_

#include <stdint.h>

namespace ola {
namespace dmx {

/**
 * @brief The minimum priority for a source.
 */
static const uint8_t SOURCE_PRIORITY_MIN = 0;

/**
 * @brief The default priority for a source.
 */
static const uint8_t SOURCE_PRIORITY_DEFAULT = 100;

/**
 * @brief The maximum priority for a source.
 */
static const uint8_t SOURCE_PRIORITY_MAX = 200;

}  // namespace dmx
}  // namespace ola
#endif  // INCLUDE_OLA_DMX_SOURCEPRIORITIES_H_
