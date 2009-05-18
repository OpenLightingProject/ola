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
 * packets.h
 * Datagram definitions for libshownet
 * Copyright (C) 2005 Simon Newton
 */

#ifndef shownet_packets_defined
#define shownet_packets_defined

#include <sys/types.h>
#include <stdint.h>
#include <netinet/in.h>

enum { SHOWNET_MAC_LENGTH = 6 };
enum { SHOWNET_NAME_LENGTH = 9 };
enum { SHOWNET_DMX_LENGTH = 512 };

struct shownet_data_s {
  uint8_t  sigHi;            // 0x80
  uint8_t  sigLo;            // 0x8f
  uint8_t  ip[4];            // ip of sender
  uint16_t netSlot[4];       // start channel of each slot
  uint16_t slotSize[4];      // size of each slot
  uint16_t indexBlock[5];    // index into data of each slot
  uint8_t  packetCountHi;    // sequence number
  uint8_t  packetCountLo;    // sequence number
  uint8_t  block[4];         // ??
  uint8_t  name[SHOWNET_NAME_LENGTH];  // name of console
  uint8_t  data[SHOWNET_DMX_LENGTH];  // data
} __attribute__( ( packed ) );

typedef struct shownet_data_s shownet_data_t;


// union of all shownet packets
typedef union {
  shownet_data_t dmx;
} shownet_packet_union_t;


// a packet, containing data, length, type and a src/dst address
typedef struct {
  int length;
  struct in_addr from;
  struct in_addr to;
  shownet_packet_union_t data;
} shownet_packet_t;

typedef shownet_packet_t *shownet_packet;

#endif
