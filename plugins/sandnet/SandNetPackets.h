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
 * SandNetPackets.h
 * Datagram definitions for SandNet
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_SANDNET_SANDNETPACKETS_H_
#define PLUGINS_SANDNET_SANDNETPACKETS_H_

#include <sys/types.h>
#include <stdint.h>
#ifndef _WIN32
#include <netinet/in.h>
#endif

#include <ola/Constants.h>
#include "ola/network/IPV4Address.h"
#include "ola/network/MACAddress.h"
#include "plugins/sandnet/SandNetCommon.h"

namespace ola {
namespace plugin {
namespace sandnet {

enum { SANDNET_NAME_LENGTH = 31};

/*
 * SandNet opcodes.
 * These are transmitted as little-endian which is why they appear strange.
 */
// We can't use the PACK macro for enums
#ifdef _WIN32
#pragma pack(push, 1)
#endif
enum packet_type_e {
  SANDNET_ADVERTISEMENT = 0x0100,
  SANDNET_CONTROL = 0x0200,
  SANDNET_DMX = 0x0300,
  SANDNET_NAME = 0x0400,
  SANDNET_IDENTIFY = 0x0500,
  SANDNET_PROG = 0x0600,
  SANDNET_LED = 0x0700,
  SANDNET_COMPRESSED_DMX = 0x0a00,
#ifdef _WIN32
};
#pragma pack(pop)
#else
} __attribute__((packed));
#endif

typedef enum packet_type_e packet_type_t;

// We can't use the PACK macro for enums
#ifdef _WIN32
#pragma pack(push, 1)
#endif
enum protocol_id_e {
  SANDNET_SANDNET = 0x02,
  SANDNET_ARTNET = 0x04,
  SANDNET_COMPULIGHT = 0x06,
  SANDNET_SHOWNET = 0x09,
  SANDNET_IPX = 0x0d,
  SANDNET_ACN = 0x0e,
#ifdef _WIN32
};
#pragma pack(pop)
#else
} __attribute__((packed));
#endif

typedef enum protocol_id_e protocol_id_t;

PACK(
struct sandnet_packet_advertisement_port_s {
  protocol_id_t protocol;  // protocol
  uint8_t mode;       // mode
  uint8_t term;       // terminate
  uint8_t b;          // ??
  uint8_t group;      // group
  uint8_t universe;   // universe
  uint8_t crap[53];   // ??
});


/*
 * A Sandnet Advertisement
 */
PACK(
struct sandnet_advertisement_s {
  uint8_t mac[ola::network::MACAddress::LENGTH];    // mac address
  uint32_t firmware;                   // firmware version
  struct sandnet_packet_advertisement_port_s ports[SANDNET_MAX_PORTS];  // ports
  uint8_t nlen;                       // length of the name field
  char name[SANDNET_NAME_LENGTH];  // name field (null terminated)
  uint8_t magic3[9];                  // magic numbers
  uint8_t led;                        // flash the led
  uint8_t magic4;                     // ??
  uint8_t zero4[64];                  // null
});

typedef struct sandnet_advertisement_s sandnet_advertisement;


/*
 * The first of the DMX packets
 */
PACK(
struct sandnet_dmx_s {
  uint8_t  group;        // group
  uint8_t  universe;     // universe
  uint8_t  port;         // physical port number
  uint8_t  dmx[DMX_UNIVERSE_SIZE];  // dmx buffer
});

typedef struct sandnet_dmx_s sandnet_dmx;


/*
 * Changes the port attributes
 */
PACK(
struct sandnet_port_control_s {
  uint8_t  mac[ola::network::MACAddress::LENGTH];  // mac address
  uint8_t  magic[4];                 // ?? seems to change
  struct sandnet_packet_advertisement_port_s ports[SANDNET_MAX_PORTS];  // ports
});

typedef struct sandnet_port_control_s sandnet_port_control;


/*
 * Sets the name of the sandnet node
 */
PACK(
struct sandnet_name_s {
  uint8_t  mac[ola::network::MACAddress::LENGTH];   // mac address
  uint8_t  name_length;               // length of the name field
  uint8_t  name[SANDNET_NAME_LENGTH];  // name field
});

typedef struct sandnet_name_s sandnet_name;


/*
 * identify packet
 * (presumably this flashes the leds or something)
 */
PACK(
struct sandnet_identify_s {
  uint8_t  mac[ola::network::MACAddress::LENGTH];  // mac address
});

typedef struct sandnet_identify_s sandnet_identify;


/*
 * ip program packet
 * sets the node's networking parameters
 */
PACK(
struct sandnet_program_s {
  uint8_t  mac[ola::network::MACAddress::LENGTH];  // mac address
  uint8_t  ip[ola::network::IPV4Address::LENGTH];
  uint8_t  dhcp;
  uint8_t  netmask[ola::network::IPV4Address::LENGTH];
});

typedef struct sandnet_program_s sandnet_program;


/*
 * Turns the led on and off
 */
PACK(
struct sandnet_led_s {
  uint8_t  mac[ola::network::MACAddress::LENGTH];  // mac address
  uint8_t  led;              // 0x00 off, 0xff on
});

typedef struct sandnet_led_s sandnet_led;


/*
 * DMX data
 */
PACK(
struct sandnet_compressed_dmx_s {
  uint8_t  group;     // group
  uint8_t  universe;  // universe
  uint8_t  port;      // physical port number
  uint8_t  zero1[4];  // could be the offset
  uint8_t  two;       // 0x02
  uint16_t length;    // length of data
  uint8_t  dmx[DMX_UNIVERSE_SIZE];
});

typedef struct sandnet_compressed_dmx_s sandnet_compressed_dmx;


// A generic Sandnet packet containing the union of all possible packets
PACK(
struct sandnet_packet {
  uint16_t opcode;
  union {
    sandnet_advertisement advertisement;
    sandnet_port_control port_control;
    sandnet_dmx dmx;
    sandnet_name name;
    sandnet_identify id;
    sandnet_program program;
    sandnet_led led;
    sandnet_compressed_dmx  compressed_dmx;
  } contents;
});
}  // namespace sandnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SANDNET_SANDNETPACKETS_H_
