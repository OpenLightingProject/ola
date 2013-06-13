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
 * BaseTypes.h
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef INCLUDE_OLA_BASETYPES_H_
#define INCLUDE_OLA_BASETYPES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  DMX_MIN_CHANNEL_VALUE = 0,
  DMX_MAX_CHANNEL_VALUE = 255,
  DMX_MIN_CHANNEL_NUMBER = 1,
  DMX_UNIVERSE_SIZE = 512
};
static const uint8_t DMX512_START_CODE = 0;
static const int OLA_DEFAULT_PORT = 9010;
static const uint16_t OPEN_LIGHTING_ESTA_CODE = 0x7a70;
static const uint32_t MAX_UNIVERSE = 0xffffffff;

/**
 * Please discuss on open-lighting@googlegroups.com before claiming additional
 * manufacturer PIDs and update http://opendmx.net/index.php/Open_Lighting_PIDs
 */
static const uint16_t MANUFACTURER_PID_SERIAL_NUMBER = 0x8000;
static const uint16_t MANUFACTURER_PID_CODE_VERSION = 0x8001;

#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_OLA_BASETYPES_H_
