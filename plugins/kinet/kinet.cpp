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
 * kinet.cpp
 * Scratch pad for Kinet work
 * Copyright (C) 2010 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/base/Init.h>
#include <ola/io/SelectServer.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>

#include <iostream>
#include <string>

using std::cout;
using std::endl;
using std::string;
using ola::io::SelectServer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::LittleEndianToHost;
using ola::network::UDPSocket;


/*
 * The KiNet protocol appears to be little-endian. We write the constants as
 * they appear to a human and convert back and forth.
 */

// All packets seem to start with this number
const uint32_t KINET_MAGIC = 0x4adc0104;
// We haven't seen a non V1 protocol in the wild yet.
const uint16_t KINET_VERSION = 0x0001;
// No idea what this is, but we should send a poll reply when we see it
const uint32_t KINET_DISCOVERY_COMMAND = 0x8988870a;

// KiNet packet types
typedef enum {
  KINET_POLL = 0x0001,
  KINET_POLL_REPLY = 0x0002,
  KINET_SET_IP = 0x0003,
  KINET_SET_UNIVERSE = 0x0005,
  KINET_SET_NAME = 0x0006,
  // KINET_?? = 0x000a;
  KINET_DMX = 0x0101,
  // KINET_PORTOUT = 0x0108;  // portout
  // KINET_PORTOUT_SYNC = 0x0109;  // portout_sync
  // KINET_?? = 0x0201;  // seems to be a discovery packet
  // KINET_?? = 0x0203;  // get dmx address?
} kinet_packet_type;


/**
 * The KiNet header
 */
PACK(
struct kinet_header {
  uint32_t magic;
  uint16_t version;
  uint16_t type;  // see kinet_packet_type above
  uint32_t padding;  // always set to 0, seq #,
                     // most of the time it's 0,
                     // not implemented in most supplies
});


// A KiNet poll message
PACK(
struct kinet_poll {
  uint32_t command;  // ??, Seems to always be KINET_DISCOVERY_COMMAND
});


// A KiNet poll reply message
PACK(
struct kinet_poll_reply {
  uint32_t src_ip;
  uint8_t hw_address[6];  // mac address
  uint8_t  data[2];  // This contains non-0 data
  uint32_t serial;  // The node serial #
  uint32_t zero;
  uint8_t node_name[60];
  uint8_t node_label[31];
  uint16_t zero2;
});


// A KiNet Set IP Command.
// TODO(simon): Check what ip,mac dst this packet is sent to.
PACK(
struct kinet_set_ip {
  uint32_t something;  // ef be ad de
  uint8_t hw_address[6];  // The MAC address to match
  uint16_t something2;  // 05 67
  uint32_t new_ip;
});


// A KiNet Set Universe Command
PACK(
struct kinet_set_universe {
  uint32_t something;  // ef be ad de
  uint8_t universe;
  uint8_t zero[3];
});


// A KiNet Set Name Command
PACK(
struct kinet_set_name {
  uint32_t something;  // ef be ad de
  uint8_t new_name[31];  // Null terminated.
});


// A KiNet Get Address command
PACK(
struct kinet_get_address {
  uint32_t serial;
  uint32_t something;  // 41 00 12 00
});


// A KiNet DMX packet
PACK(
struct kinet_dmx {
  uint8_t port;  // should be set to 0 for v1
  uint8_t flags;  // set to 0
  uint16_t timerVal;  // set to 0
  uint32_t universe;
  uint8_t paylod[513];  // payload inc start code
});


struct kinet_port_out_flags {
  uint16_t flags;
    // little endian
    // first bit is undefined  0:1;
    // second bit is for 16 bit data, set to 0  :1;
    // third is shall hold for sync packet :: 1;
};


struct kinet_port_out_sync {
  uint32_t padding;
};

// A KiNet DMX port out packet
PACK(
struct kinet_port_out {
  uint32_t universe;
  uint8_t port;        // 1- 16
  uint8_t pad;         // set to 0
  kinet_port_out_flags flags;
  uint16_t length;     // little endian
  uint16_t startCode;  // 0x0fff for chomASIC products, 0x0000 otherwise
  uint8_t payload[512];
});


// The full kinet packet
struct kinet_packet {
  struct kinet_header header;
  union {
    struct kinet_poll poll;
    struct kinet_poll_reply poll_reply;
  } data;
};


uint8_t peer0_0[] = {
  0x04, 0x01, 0xdc, 0x4a,  // magic number
  0x01, 0x00,  // version #
  0x02, 0x00,  // packet type (poll reply)
  0x00, 0x00, 0x00, 0x00,  // sequence
  0x0a, 0x00, 0x00, 0x01,  // 192.168.1.207
  0x00, 0x0a, 0xc5, 0xff, 0xae, 0x01,  // mac address
  0x01, 0x00,
  0xff, 0xff, 0x00, 0x2d,  // serial #
  0x00, 0x00, 0x00, 0x00,  // padding
  // What follows is ascii text, with fields separated by new lines. Each field
  // is in the form /[MD#R]:.*/
  // It's unclear is this is a variable length field or not.
  0x4d, 0x3a,  // M:
  0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x20, 0x4b, 0x69, 0x6e, 0x65, 0x74, 0x69, 0x63,
  0x73, 0x20, 0x49, 0x6e, 0x63, 0x6f, 0x72, 0x70, 0x6f, 0x72, 0x61, 0x74, 0x65,
  0x64,  // Color Kinetics cs Incorporated
  0x0a,  // \n
  0x44, 0x3a,  // D:
  0x50, 0x44, 0x53, 0x2d, 0x65,  // PDS-e
  0x0a,  // \n
  0x23, 0x3a,  // #:
  0x53, 0x46, 0x54, 0x2d, 0x30, 0x30, 0x30, 0x30, 0x36, 0x36, 0x2d, 0x30, 0x30,
  0x0a,  // SFT-000066-00
  0x52, 0x3a,  // R:
  0x30, 0x30,  // 00
  0x0a,  // \n
  0x00,
  // offset 92
  0x64, 0x73, 0x2d, 0x64, 0x61, 0x6e, 0x63, 0x65, 0x2d, 0x72, 0x65, 0x61, 0x72,
  0x00,  // device name?
  0x00, 0x95, 0x8c, 0xc7, 0xb6, 0x00,
  0x00,
  0xff, 0x00, 0x00,
  0xff, 0x00, 0x00,
  0xff, 0x00, 0x00,
  0xff, 0x00, 0x00 };


SelectServer ss;
UDPSocket udp_socket;

/**
 * Check if a packet is valid KiNet
 */
bool IsKiNet(const kinet_packet *packet, unsigned int size) {
  return (size > sizeof(struct kinet_header) &&
          KINET_MAGIC == LittleEndianToHost(packet->header.magic) &&
          KINET_VERSION == LittleEndianToHost(packet->header.version));
}


/**
 * Handle a KiNet poll packet
 */
void HandlePoll(const IPV4SocketAddress &source,
                OLA_UNUSED const kinet_packet &packet,
                OLA_UNUSED unsigned int size) {
  ssize_t r = udp_socket.SendTo(peer0_0, sizeof(peer0_0), source);
  OLA_INFO << "sent " << r << " bytes";
}


/**
 * Handle a KiNet DMX packet
 */
void HandleDmx(OLA_UNUSED const IPV4SocketAddress &source,
               OLA_UNUSED const kinet_packet &packet,
               OLA_UNUSED unsigned int size) {
}


void SocketReady() {
  kinet_packet packet;
  ssize_t data_read = sizeof(packet);
  IPV4SocketAddress source;

  udp_socket.RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                      &data_read,
                      &source);
  if (IsKiNet(&packet, data_read)) {
    uint16_t command = LittleEndianToHost(packet.header.type);
    switch (command) {
      case KINET_POLL:
        HandlePoll(source, packet, data_read);
        break;
      case KINET_DMX:
        HandleDmx(source, packet, data_read);
        break;
      default:
        OLA_WARN << "Unknown packet 0x" << std::hex << command;
    }
  } else {
    OLA_WARN << "Not a KiNet packet";
  }
}


/*
 * Main
 */
int main(int argc, char *argv[]) {
  ola::AppInit(&argc, argv, "", "Run the Kinet scratchpad.");

  udp_socket.SetOnData(ola::NewCallback(&SocketReady));
  if (!udp_socket.Init()) {
    OLA_WARN << "Failed to init";
    return 1;
  }
  if (!udp_socket.Bind(IPV4SocketAddress(IPV4Address::WildCard(), 6038))) {
    OLA_WARN << "Failed to bind";
    return 1;
  }
  if (!udp_socket.EnableBroadcast()) {
    OLA_WARN << "Failed to enabl bcast";
    return 1;
  }

  ss.AddReadDescriptor(&udp_socket);

  ss.Run();
  return 0;
}
