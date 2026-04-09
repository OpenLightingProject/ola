/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ACNFlags.h
 * Flags used in ACN PDUs
 * Copyright (C) 2020 Peter Newman
 */

#ifndef INCLUDE_OLA_ACN_ACNFLAGS_H_
#define INCLUDE_OLA_ACN_ACNFLAGS_H_

/**
 * @addtogroup acn
 * @{
 * @file ACNFlags.h
 * @brief ACN flag values.
 * @}
 */

#include <stdint.h>

namespace ola {
namespace acn {

/**
 * @addtogroup acn
 * @{
 */

// masks for the flag fields
/**
 * @brief This indicates a 20 bit length field (default is 12 bits)
 */
static const uint8_t LFLAG_MASK = 0x80;

/**
 * @brief This indicates a vector is present
 */
static const uint8_t VFLAG_MASK = 0x40;

/**
 * @brief This indicates a header field is present
 */
static const uint8_t HFLAG_MASK = 0x20;

/**
 * @brief This indicates a data field is present
 */
static const uint8_t DFLAG_MASK = 0x10;

/**
 * @}
 */

}  // namespace acn
}  // namespace ola

#endif  // INCLUDE_OLA_ACN_ACNFLAGS_H_
