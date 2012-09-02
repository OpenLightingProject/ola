/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SLPPacketConstants.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPPACKETCONSTANTS_H_
#define TOOLS_SLP_SLPPACKETCONSTANTS_H_

#include <stdint.h>

namespace ola {
namespace slp {

static const uint8_t SLP_VERSION = 2;

typedef uint16_t xid_t;

typedef enum {
  SERVICE_REQUEST = 1,
  SERVICE_REPLY = 2,
  SERVICE_REGISTRATION = 3,
  SERVICE_DEREGISTER = 4,
  SERVICE_ACKNOWLEDGE = 5,
  ATTRIBUTE_REQUEST = 6,
  ATTRIBUTE_REPLY = 7,
  DA_ADVERTISEMENT = 8,
  SERVICE_TYPE_REQUEST = 9,
  SERVICE_TYPE_REPLY = 10,
  SA_ADVERTISEMENT = 11,
  MAX_SLP_FUNCTION_ID = 12,
} slp_function_id_t;

static const uint16_t SLP_FRESH = 0x40;
static const uint16_t SLP_OVERFLOW = 0x80;
static const uint16_t SLP_REQUEST_MCAST = 0x20;
static const uint8_t EN_LANGUAGE_TAG[] = {'e', 'n'};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPPACKETCONSTANTS_H_
