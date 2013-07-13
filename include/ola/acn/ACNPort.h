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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ACNPort.h
 * Contains the UDP ports for ACN
 * Copyright (C) 2011 Simon Newton
 */

/**
 * @defgroup acn ACN
 * @brief Architecture for Control Networks
 *
 * ACN is a suite of ANSI Standard protocols for transporting lighting control
 * data. See
 * [ACN on
 * wikipedia](http://en.wikipedia.org/wiki/Architecture_for_Control_Networks).
 *
 * This emcompasses code for E1.31 (Streaming ACN) and E1.33 (RDMNet).
 */

/**
 * @addtogroup acn
 * @{
 * @file ACNPort.h
 * @brief The TCP / UDP Ports used for transporting ACN.
 */

#ifndef INCLUDE_OLA_ACN_ACNPORT_H_
#define INCLUDE_OLA_ACN_ACNPORT_H_

#include <stdint.h>

namespace ola {

/**
 * @brief ACN related code.
 */
namespace acn {

/**
 * @brief The port used for E1.31 & SDT communication.
 */
const uint16_t ACN_PORT = 5568;

/**
 * @brief The port used for E1.33 communication.
 */
const uint16_t E133_PORT = 5569;
}  // namespace acn
}  // namespace ola

/**
 * @}
 */
#endif  // INCLUDE_OLA_ACN_ACNPORT_H_
