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

#include <algorithm>
#include <string>
#include <vector>
#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>

#include "tools/slp/SLPPacketParser.h"

using ola::io::IOQueue;
using ola::network::IPV4Address;
using ola::network::NetworkToHost;
using std::string;
using std::vector;

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
    const uint8_t *data,
    unsigned int length) const {
  SLPPacket header;
  unsigned int data_offset = 0;
  if (!ExtractHeader(data, length, &header, &data_offset))
    return NULL;

  // now try to extract the data

  return NULL;
}


/**
 * Check the contents of the header, and populate the packet structure.
 * Returns true if this packet is valid, false otherwise. data_offset is set to
 * the first bute of the packet contents.
 */
bool SLPPacketParser::ExtractHeader(const uint8_t *data,
                                    unsigned int length,
                                    SLPPacket *packet,
                                    unsigned int *data_offset) const {
  if (length < sizeof(slp_header_t)) {
    OLA_INFO << "SLP Packet too small, was " << length << ", required " <<
      sizeof(slp_header_t);
    return false;
  }
  const slp_header_t *header = reinterpret_cast<const slp_header_t*>(data);

  if (header->version != SLP_VERSION) {
    OLA_INFO << "Invalid SLP Version: " << static_cast<int>(header->version);
    return false;
  }

  unsigned int packet_length = (NetworkToHost(header->length) << 8) +
    header->length_lo;
  if (packet_length != length) {
    OLA_INFO << "SLP Packet length mismatch, header said " << packet_length <<
      ", packet size was " << length;
    return false;
  }

  packet->flags = NetworkToHost(header->flags);

  unsigned int next_ext_offset = header->next_ext_offset_hi << 16 +
    NetworkToHost(header->next_ext_offset);
  if (next_ext_offset > length) {
    OLA_INFO << "Next Ext. Offset of " << next_ext_offset <<
      ", is larger than the packet length of " << length;
    return false;
  }

  packet->xid = NetworkToHost(header->xid);
  unsigned int lang_tag_length = NetworkToHost(header->lang_tag_size);

  if (lang_tag_length > length - sizeof(slp_header_t)) {
    OLA_INFO << "Lang Tag length of " << lang_tag_length <<
      " exceeds the remaining SLP Packet size";
    return false;
  }

  uint8_t offset = sizeof(slp_header_t);
  packet->language = string(reinterpret_cast<const char*>(data + offset),
                            lang_tag_length);
  *data_offset = offset + lang_tag_length;
  return true;
}


/**
 * Attempt to read a string from a data buffer. The first two bytes are the
 * string length.
 */
bool SLPPacketParser::ReadString(uint8_t *data,
                                 unsigned int length,
                                 const string &field_name,
                                 string *result) const {
  if (length < 2) {
    OLA_INFO << "Packet too small to read " << field_name << " length";
    return false;
  }
  uint16_t str_length;
  memcpy(reinterpret_cast<uint8_t*>(&str_length), data, sizeof(str_length));
  str_length = NetworkToHost(str_length);
  data += sizeof(str_length);
  length -= sizeof(str_length);

  if (str_length > length) {
    OLA_INFO << "Insufficent data remaining for SLP string, expected " <<
      str_length << ", " << length << " remaining";
    return false;
  }

  *result = string(reinterpret_cast<const char*>(data), str_length);
  return true;
}
}  // slp
}  // ola
