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
 * PathportPackets.h
 * Datagram definitions for libpathport
 * Copyright (C) 2006-2009 Simon Newton
 */

#ifndef PLUGINS_PATHPORT_PATHPORTPACKETS_H_
#define PLUGINS_PATHPORT_PATHPORTPACKETS_H_

#include <sys/types.h>
#include <stdint.h>
#include "ola/network/IPV4Address.h"

namespace ola {
namespace plugin {
namespace pathport {


/*
 * Pathport opcodes
 */
enum pathport_packet_type_e {
  PATHPORT_DATA = 0x0100,
  PATHPORT_PATCH = 0x0200,
  PATHPORT_PATCHREP = 0x0210,
  PATHPORT_GET = 0x0222,
  PATHPORT_GET_REPLY = 0x0223,
  PATHPORT_ARP_REQUEST = 0x0301,
  PATHPORT_ARP_REPLY = 0x0302,
  PATHPORT_SET = 0x0400,
}__attribute__((packed));

typedef enum pathport_packet_type_e pathport_packet_type_t;


/*
 * Pathport xDmx
 */
struct pathport_pdu_data_s {
  uint16_t type;
  uint16_t channel_count;
  uint8_t universe;  // not used, set to 0
  uint8_t start_code;
  uint16_t offset;
  uint8_t data[0];
}__attribute__((packed));

typedef struct pathport_pdu_data_s pathport_pdu_data;


/*
 * Pathport get request
 */
struct pathport_pdu_get_s {
  uint16_t params[0];
} __attribute__((packed));

typedef struct pathport_pdu_get_s pathport_pdu_get;


/*
 * Pathport get reply
 */
struct pathport_pdu_getrep_s {
  uint8_t params[0];
} __attribute__((packed));

typedef struct pathport_pdu_getrep_s pathport_pdu_getrep;


struct pathport_pdu_getrep_alv_s {
  uint16_t type;
  uint16_t len;
  uint8_t val[0];
};
typedef struct pathport_pdu_getrep_alv_s pathport_pdu_getrep_alv;


/*
 * Pathport arp reply
 */
struct pathport_pdu_arp_reply_s {
  uint32_t id;
  uint8_t ip[ola::network::IPV4Address::LENGTH];
  uint8_t manufacturer_code;  // manufacturer code
  uint8_t device_class;  // device class
  uint8_t device_type;  // device type
  uint8_t component_count;  // number of dmx components
}__attribute__((packed));

typedef struct pathport_pdu_arp_reply_s pathport_pdu_arp_reply;


struct pathport_pdu_header_s {
  uint16_t type;    // pdu type
  uint16_t len;    // length
}__attribute__((packed));

typedef struct pathport_pdu_header_s pathport_pdu_header;

/*
 * PDU Header
 */
struct pathport_packet_pdu_s {
  pathport_pdu_header head;
  union {
    pathport_pdu_data data;
    pathport_pdu_get get;
    pathport_pdu_getrep getrep;
    pathport_pdu_arp_reply arp_reply;
  } d;    // pdu data
}__attribute__((packed));

typedef struct pathport_packet_pdu_s pathport_packet_pdu;


/*
 * A complete Pathport packet
 */
struct pathport_packet_header_s {
  uint16_t protocol;
  uint8_t version_major;
  uint8_t version_minor;
  uint16_t sequence;
  uint8_t reserved[6];  // set to 0
  uint32_t source;  // src id
  uint32_t destination;  // dst id
}__attribute__((packed));

typedef struct pathport_packet_header_s pathport_packet_header;


/*
 * The complete pathport packet
 */
struct pathport_packet_s {
  pathport_packet_header header;
  union {
    uint8_t data[1480];  // 1500 - header size
    pathport_packet_pdu pdu;
  } d;
}__attribute__((packed));
}  // namespace pathport
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_PATHPORT_PATHPORTPACKETS_H_
