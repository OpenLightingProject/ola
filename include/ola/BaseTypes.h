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
 * BaseTypes.h
 * Copyright (C) 2005 Simon Newton
 */

/**
 * @file BaseTypes.h
 * @deprecated Use Constants.h instead.
 */

#ifndef INCLUDE_OLA_BASETYPES_H_
#define INCLUDE_OLA_BASETYPES_H_

#include <stdint.h>
#include <ola/Constants.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The number of slots in a DMX512 universe.
 * @deprecated Use ola::DMX_UNIVERSE_SIZE from Constants.h instead.
 */
enum {
  DMX_UNIVERSE_SIZE = ola::DMX_UNIVERSE_SIZE,
};

/**
 * @brief The minimum value a DMX512 slot can take.
 * @deprecated Use ola::DMX_MIN_SLOT_VALUE from Constants.h instead.
 */
static const uint8_t DMX_MIN_CHANNEL_VALUE = ola::DMX_MIN_SLOT_VALUE;

/**
 * @brief The maximum value a DMX512 slot can take.
 * @deprecated Use ola::DMX_MAX_SLOT_VALUE from Constants.h instead.
 */
static const uint8_t DMX_MAX_CHANNEL_VALUE = ola::DMX_MAX_SLOT_VALUE;

/**
 * @brief The start code for DMX512 data.
 *
 * This is often referred to as NSC for "Null Start Code".
 * @deprecated Use ola::DMX512_START_CODE from Constants.h instead.
 */
static const uint8_t DMX512_START_CODE = ola::DMX512_START_CODE;

/**
 * @brief The default port which olad listens on for incoming RPC connections.
 * @deprecated Use ola::OLA_DEFAULT_PORT from Constants.h instead.
 */
static const int OLA_DEFAULT_PORT = ola::OLA_DEFAULT_PORT;

/**
 * @brief The ESTA manufacturer code for the Open Lighting Project.
 * @deprecated Use ola::OPEN_LIGHTING_ESTA_CODE from Constants.h instead.
 */
static const uint16_t OPEN_LIGHTING_ESTA_CODE = ola::OPEN_LIGHTING_ESTA_CODE;

/**
 * @brief The maximum universe number.
 * @deprecated Use ola::MAX_UNIVERSE from Constants.h instead.
 */
static const uint32_t MAX_UNIVERSE = ola::MAX_UNIVERSE;

#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_OLA_BASETYPES_H_
