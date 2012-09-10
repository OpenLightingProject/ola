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
 * SLPPacketParser.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/io/BigEndianStream.h>
#include <ola/network/NetworkUtils.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "tools/slp/SLPPacketParser.h"

using ola::io::BigEndianInputStream;
using ola::network::IPV4Address;
using ola::network::NetworkToHost;
using std::string;
using std::vector;
using std::auto_ptr;

namespace ola {
namespace slp {

/*
 * Return the function-id for a packet, or 0 if the packet is malformed.
 */
uint8_t SLPPacketParser::DetermineFunctionID(const uint8_t *data,
                                             unsigned int length) const {
  if (length < 2) {
    OLA_WARN << "SLP Packet too short to extract function-id";
    return 0;
  }
  return data[1];
};


/**
 * Unpack a Service Request, assumes the Function-ID is SERVICE_REQUEST
 */
const ServiceRequestPacket* SLPPacketParser::UnpackServiceRequest(
    BigEndianInputStream *input) const {
  auto_ptr<ServiceRequestPacket> packet(new ServiceRequestPacket());
  if (!ExtractHeader(input, packet.get()))
    return NULL;

  // now try to extract the data


  return packet.release();
}


/**
 * Check the contents of the header, and populate the packet structure.
 * Returns true if this packet is valid, false otherwise.
 */
bool SLPPacketParser::ExtractHeader(BigEndianInputStream *input,
                                    SLPPacket *packet) const {
  /*
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |    Version    |  Function-ID  |            Length             |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     | Length, contd.|O|F|R|       reserved          |Next Ext Offset|
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |  Next Extension Offset, contd.|              XID              |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |      Language Tag Length      |         Language Tag          \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */

  // we shouldn't see failure for the version & function since everything is
  // passed through DetermineFunctionID() above.
  uint8_t version, function_id;
  if (!(*input >> version)) {
    OLA_INFO << "SLP Packet too small to contain version number";
    return false;
  }

  if (version != SLP_VERSION) {
    OLA_INFO << "Invalid SLP Version: " << static_cast<int>(version);
    return false;
  }

  if (!(*input >> function_id)) {
    OLA_INFO << "SLP Packet too small to contain function id";
    return false;
  }

  // ok, now we've got new stuff
  uint16_t length_hi;
  uint8_t length_lo;
  if (!(*input >> length_hi && *input >> length_lo)) {
    OLA_INFO << "SLP Packet too small to contain length";
    return false;
  }

  unsigned int packet_length = length_hi << 8 + length_lo;
  (void) packet_length;
  /*
  // There is no Size() methods for InputStreams so we can't check this.
  if (packet_length != length) {
    OLA_INFO << "SLP Packet length mismatch, header said " << packet_length <<
      ", packet size was " << length;
    return false;
  }
  */

  if (!(*input >> packet->flags)) {
    OLA_INFO << "SLP Packet too small to contain flags";
    return false;
  }

  uint8_t next_ext_hi;
  uint16_t next_ext_lo;
  if (!(*input >> next_ext_hi && *input >> next_ext_lo)) {
    OLA_INFO << "SLP Packet too small to contain Next Ext. Offset";
    return false;
  }

  unsigned int next_ext_offset = next_ext_hi << 16 + next_ext_lo;
  if (next_ext_offset)
    OLA_INFO << "Next Ext non-0, was " << next_ext_offset;

  if (!(*input >> packet->xid)) {
    OLA_INFO << "SLP Packet too small to contain XID";
    return false;
  }

  if (!ReadString(input, "Language", &packet->language))
    return false;
  return true;
}


/**
 * Attempt to read a string from a data buffer. The first two bytes are the
 * string length.
 */
bool SLPPacketParser::ReadString(BigEndianInputStream *input,
                                 const string &field_name,
                                 string *result) const {
  uint16_t str_length;
  if (!(*input >> str_length)) {
    OLA_INFO << "Packet too small to read " << field_name << " length";
    return false;
  }

  unsigned int bytes_read = input->ReadString(result, str_length);
  if (bytes_read != str_length) {
    OLA_INFO << "Insufficent data remaining for SLP string " << field_name <<
      ", expected " << str_length << ", " << bytes_read << " remaining";
    return false;
  }
  return true;
}
}  // slp
}  // ola
