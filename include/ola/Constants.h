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
 * Constants.h
 * Copyright (C) 2005 Simon Newton
 */

/**
 * @file Constants.h
 * @brief Constants used throughout OLA.
 */

#ifndef INCLUDE_OLA_CONSTANTS_H_
#define INCLUDE_OLA_CONSTANTS_H_

#include <stdint.h>

namespace ola {

/**
 * @brief The number of slots in a DMX512 universe.
 */
enum {
  DMX_UNIVERSE_SIZE = 512  /**< The number of slots in a DMX512 universe. */
};

/**
 * @brief The minimum value a DMX512 slot can take.
 */
static const uint8_t DMX_MIN_SLOT_VALUE = 0;

/**
 * @brief The maximum value a DMX512 slot can take.
 */
static const uint8_t DMX_MAX_SLOT_VALUE = 255;

/**
 * @brief The start code for DMX512 data.
 * This is often referred to as NSC for "Null Start Code".
 */
static const uint8_t DMX512_START_CODE = 0;

/**
 * @brief The default port which olad listens on for incoming RPC connections.
 */
static const int OLA_DEFAULT_PORT = 9010;

/**
 * @brief The default instance name for olad.
 */
static const char OLA_DEFAULT_INSTANCE_NAME[] = "OLA Server";

/**
 * @brief The ESTA manufacturer code for the Open Lighting Project.
 */
static const uint16_t OPEN_LIGHTING_ESTA_CODE = 0x7a70;

/**
 * @brief The maximum universe number.
 */
static const uint32_t MAX_UNIVERSE = 0xffffffff;

}  // namespace ola

#endif  // INCLUDE_OLA_CONSTANTS_H_
