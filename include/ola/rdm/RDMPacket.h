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
 * RDMPacket.h
 * Structures and constants used with RDM Packets.
 * Copyright (C) 2005-2012 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RDMPACKET_H_
#define INCLUDE_OLA_RDM_RDMPACKET_H_

#include <stdint.h>
#include <ola/rdm/UID.h>

namespace ola {
namespace rdm {

static const uint8_t START_CODE = 0xcc;
static const uint8_t SUB_START_CODE = 0x01;
static const unsigned int CHECKSUM_LENGTH = 2;


// Don't use anything other than uint8_t here otherwise we can get alignment
// issues.
typedef struct {
  uint8_t sub_start_code;
  uint8_t message_length;
  uint8_t destination_uid[UID::UID_SIZE];
  uint8_t source_uid[UID::UID_SIZE];
  uint8_t transaction_number;
  uint8_t port_id;
  uint8_t message_count;
  uint8_t sub_device[2];
  uint8_t command_class;
  uint8_t param_id[2];
  uint8_t param_data_length;
} RDMCommandHeader;
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_RDMPACKET_H_
