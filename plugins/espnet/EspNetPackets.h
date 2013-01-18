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
 * EspNetPackets.h
 * Datagram definitions for EspNet
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef PLUGINS_ESPNET_ESPNETPACKETS_H_
#define PLUGINS_ESPNET_ESPNETPACKETS_H_

#include <sys/types.h>
#include <stdint.h>
#include <netinet/in.h>

#include "ola/network/InterfacePicker.h"  // MAC_LENGTH
#include "ola/BaseTypes.h"

namespace ola {
namespace plugin {
namespace espnet {

enum { ESPNET_NAME_LENGTH = 10 };
enum { ESPNET_DATA_LENGTH = 200 };

enum espnet_packet_type_e {
  ESPNET_POLL = 'E' << 24 | 'S' << 16 | 'P' << 8 | 'P',
  ESPNET_REPLY = 'E' << 24 | 'S' << 16 | 'P' << 8 | 'R',
  ESPNET_DMX = 'E' << 24 | 'S' << 16 | 'D' << 8 | 'D',
  ESPNET_ACK = 'E' << 24 | 'S' << 16 | 'A' << 8 | 'P'
}__attribute__((packed));

typedef enum espnet_packet_type_e espnet_packet_type_t;

/*
 * poll datagram
 */
struct espnet_poll_s {
  uint32_t head;
  uint8_t  type;
} __attribute__((packed));

typedef struct espnet_poll_s espnet_poll_t;


/*
 * This is used in the poll reply and config
 */
struct espnet_node_config_s {
  uint8_t listen;
  uint8_t ip[4];
  uint8_t universe;  // bit bizzare that nodes only listen to one universe??
};

typedef struct espnet_node_config_s espnet_node_config_t;

/*
 * poll reply
 */
struct espnet_poll_reply_s {
  uint32_t head;
  uint8_t  mac[ola::network::MAC_LENGTH];
  uint16_t type;
  uint8_t  version;
  uint8_t  sw;
  uint8_t  name[ESPNET_NAME_LENGTH];
  uint8_t  option;
  uint8_t  tos;
  uint8_t  ttl;
  espnet_node_config_t config;
} __attribute__((packed));

typedef struct espnet_poll_reply_s espnet_poll_reply_t;

/*
 * ack datagram
 */
struct espnet_ack_s {
  uint32_t head;
  uint8_t  status;
  uint8_t  crc;
} __attribute__((packed));

typedef struct espnet_ack_s espnet_ack_t;

/*
 * dmx datagram
 */
struct espnet_data_s {
  uint32_t head;
  uint8_t  universe;
  uint8_t  start;
  uint8_t  type;
  uint16_t size;
  uint8_t  data[DMX_UNIVERSE_SIZE];
} __attribute__((packed));

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
}  // espnet
}  // plugin
}  // ola
#endif  // PLUGINS_ESPNET_ESPNETPACKETS_H_
