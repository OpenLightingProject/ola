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
 * ShowNetPackets.h
 * Datagram definitions for libshownet
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef OLA_SHOWNET_PACKETS
#define OLA_SHOWNET_PACKETS

#include <ola/BaseTypes.h>

enum { SHOWNET_MAC_LENGTH = 6 };
enum { SHOWNET_NAME_LENGTH = 9 };

struct shownet_data_s {
  uint8_t  sigHi;            // 0x80
  uint8_t  sigLo;            // 0x8f
  uint8_t  ip[4];            // ip of sender
  uint16_t netSlot[4];       // start channel of each slot
  uint16_t slotSize[4];      // size of each slot
  uint16_t indexBlock[5];    // index into data of each slot
  uint8_t  packetCountHi;    // sequence number
  uint8_t  packetCountLo;    // sequence number
  uint8_t  block[4];         // the last 2 items here have something to do with
                             // the channels that have passwords are.
  uint8_t  name[SHOWNET_NAME_LENGTH]; // name of console
  uint8_t  data[DMX_UNIVERSE_SIZE]; // data
} __attribute__((packed));

typedef struct shownet_data_s shownet_data_packet;

#endif
