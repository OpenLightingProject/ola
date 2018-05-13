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
 * EspNetPackets.h
 * Datagram definitions for EspNet
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_ESPNET_ESPNETPACKETS_H_
#define PLUGINS_ESPNET_ESPNETPACKETS_H_

#include <sys/types.h>
#include <stdint.h>
#ifndef _WIN32
#include <netinet/in.h>
#endif  // _WIN32

#include "ola/network/MACAddress.h"
#include "ola/Constants.h"

namespace ola {
namespace plugin {
namespace espnet {

enum { ESPNET_NAME_LENGTH = 10 };
enum { ESPNET_DATA_LENGTH = 200 };

// We can't use the PACK macro for enums
#ifdef _WIN32
#pragma pack(push, 1)
#endif  // _WIN32
enum espnet_packet_type_e {
  ESPNET_POLL = 'E' << 24 | 'S' << 16 | 'P' << 8 | 'P',
  ESPNET_REPLY = 'E' << 24 | 'S' << 16 | 'P' << 8 | 'R',
  ESPNET_DMX = 'E' << 24 | 'S' << 16 | 'D' << 8 | 'D',
  ESPNET_ACK = 'E' << 24 | 'S' << 16 | 'A' << 8 | 'P'
#ifdef _WIN32
};
#pragma pack(pop)
#else
} __attribute__((packed));
#endif  // _WIN32

typedef enum espnet_packet_type_e espnet_packet_type_t;

/*
 * poll datagram
 */
PACK(
struct espnet_poll_s {
  uint32_t head;
  uint8_t  type;
});

typedef struct espnet_poll_s espnet_poll_t;


/*
 * This is used in the poll reply and config
 */
struct espnet_node_config_s {
  uint8_t listen;
  uint8_t ip[4];
  uint8_t universe;  // bit bizarre that nodes only listen to one universe??
};

typedef struct espnet_node_config_s espnet_node_config_t;

/*
 * poll reply
 */
PACK(
struct espnet_poll_reply_s {
  uint32_t head;
  uint8_t  mac[ola::network::MACAddress::LENGTH];
  uint16_t type;
  uint8_t  version;
  uint8_t  sw;
  uint8_t  name[ESPNET_NAME_LENGTH];
  uint8_t  option;
  uint8_t  tos;
  uint8_t  ttl;
  espnet_node_config_t config;
});

typedef struct espnet_poll_reply_s espnet_poll_reply_t;

/*
 * ack datagram
 */
PACK(
struct espnet_ack_s {
  uint32_t head;
  uint8_t  status;
  uint8_t  crc;
});

typedef struct espnet_ack_s espnet_ack_t;

/*
 * dmx datagram
 */
PACK(
struct espnet_data_s {
  uint32_t head;
  uint8_t  universe;
  uint8_t  start;
  uint8_t  type;
  uint16_t size;
  uint8_t  data[DMX_UNIVERSE_SIZE];
});

typedef struct espnet_data_s espnet_data_t;


// we need to add the TCP config crap here


/*
 * union of all espnet packets
 */
typedef union {
  espnet_poll_t poll;
  espnet_poll_reply_t reply;
  espnet_ack_t ack;
  espnet_data_t dmx;
} espnet_packet_union_t;
}  // namespace espnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_ESPNET_ESPNETPACKETS_H_
