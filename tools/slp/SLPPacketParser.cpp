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
#include <ola/StringUtils.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/SLPStrings.h"

using ola::StringSplit;
using ola::io::BigEndianInputStream;
using ola::network::IPV4Address;
using ola::network::NetworkToHost;
using std::auto_ptr;
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
    BigEndianInputStream *input) const {
  auto_ptr<ServiceRequestPacket> packet(new ServiceRequestPacket());
  if (!ExtractHeader(input, packet.get(), "SrvRqst"))
    return NULL;

  // now try to extract the data
  string pr_list;
  if (!ExtractString(input, &pr_list, "PR List"))
    return NULL;
  ConvertIPAddressList(pr_list, &packet->pr_list);

  if (!ExtractString(input, &packet->service_type, "Service Type"))
    return NULL;

  if (!ExtractList(input, &packet->scope_list, "Scope list"))
    return NULL;

  if (!ExtractString(input, &packet->predicate, "Predicate"))
    return NULL;

  if (!ExtractString(input, &packet->spi, "SPI String"))
    return NULL;

  return packet.release();
}


/**
 * Unpack a Service Reply messages
 */
const ServiceReplyPacket* SLPPacketParser::UnpackServiceReply(
    BigEndianInputStream *input) const {
  auto_ptr<ServiceReplyPacket> packet(new ServiceReplyPacket());
  if (!ExtractHeader(input, packet.get(), "SrvRply"))
    return NULL;

  if (!ExtractValue(input, &packet->error_code, "SrvRply: Error Code"))
    return NULL;

  uint16_t url_entry_count;
  if (!ExtractValue(input, &url_entry_count, "SrvRply: URL Entry Count"))
    return NULL;

  URLEntry entry;
  for (unsigned int i = 0; i < url_entry_count; i++) {
    if (ExtractURLEntry(input, &entry, "SrvRply"))
      packet->url_entries.push_back(entry);
    else
      break;
  }

  return packet.release();
}


/**
 * Unpack a Service Registration packet
 */
const ServiceRegistrationPacket *SLPPacketParser::UnpackServiceRegistration(
    BigEndianInputStream *input) const {
  auto_ptr<ServiceRegistrationPacket> packet(new ServiceRegistrationPacket());
  if (!ExtractHeader(input, packet.get(), "SrvReg"))
    return NULL;

  if (!ExtractURLEntry(input, &packet->url, "SrvReg"))
    return NULL;

  if (!ExtractString(input, &packet->service_type, "Service-type"))
    return NULL;

  if (!ExtractList(input, &packet->scope_list, "Scope list"))
    return NULL;

  if (!ExtractString(input, &packet->attr_list, "Attr-list"))
    return NULL;

  uint8_t url_auths;
  if (!ExtractValue(input, &url_auths, "SrvReg: # of URL Auths"))
    return NULL;

  for (unsigned int i = 0; i < url_auths; i++) {
    if (!ExtractAuthBlock(input, "SrvReg"))
      return NULL;
  }
  return packet.release();
}


/**
 * Unpack a Service Ack packet
 */
const ServiceAckPacket *SLPPacketParser::UnpackServiceAck(
    BigEndianInputStream *input) const {
  auto_ptr<ServiceAckPacket> packet(new ServiceAckPacket());
  if (!ExtractHeader(input, packet.get(), "SrvAck"))
    return NULL;

  if (!ExtractValue(input, &packet->error_code, "SrvAck: error-code"))
    return NULL;
  return packet.release();
}


/**
 * Unpack a DAAdvert packet
 */
const DAAdvertPacket *SLPPacketParser::UnpackDAAdvert(
    BigEndianInputStream *input) const {
  auto_ptr<DAAdvertPacket> packet(new DAAdvertPacket());
  if (!ExtractHeader(input, packet.get(), "DAAdvert"))
    return NULL;

  if (!ExtractValue(input, &packet->error_code, "DAAdvert: error-code"))
    return NULL;

  if (!ExtractValue(input, &packet->boot_timestamp, "DAAdvert: boot-timestamp"))
    return NULL;

  if (!ExtractString(input, &packet->url, "DAAdvert: URL"))
    return NULL;

  if (!ExtractList(input, &packet->scope_list, "DAAdvert: Scope list"))
    return NULL;

  if (!ExtractString(input, &packet->attr_list, "DAAdvert: Attr-list"))
    return NULL;

  string spi_string;
  if (!ExtractString(input, &spi_string, "DAAdvert: SPI String"))
    return false;

  uint8_t url_auths;
  if (!ExtractValue(input, &url_auths, "DAAdvert: # of URL Auths"))
    return NULL;

  for (unsigned int i = 0; i < url_auths; i++) {
    if (!ExtractAuthBlock(input, "DAAdvert"))
      return NULL;
  }
  return packet.release();
}


/**
 * Check the contents of the header, and populate the packet structure.
 * Returns true if this packet is valid, false otherwise.
 */
bool SLPPacketParser::ExtractHeader(BigEndianInputStream *input,
                                    SLPPacket *packet,
                                    const string &packet_type) const {
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
    OLA_INFO << packet_type << " too small to contain length";
    return false;
  }

  unsigned int packet_length = length_hi << 8 + length_lo;
  (void) packet_length;
  // There is no Size() methods for InputStreams so we can't check this.
  /*
  if (packet_length != length) {
    OLA_INFO << "SLP Packet length mismatch, header said " << packet_length <<
      ", packet size was " << length;
    return false;
  }
  */

  if (!(*input >> packet->flags)) {
    OLA_INFO << packet_type << " too small to contain flags";
    return false;
  }

  uint8_t next_ext_hi;
  uint16_t next_ext_lo;
  if (!(*input >> next_ext_hi && *input >> next_ext_lo)) {
    OLA_INFO << packet_type << " too small to contain Next Ext. Offset";
    return false;
  }

  unsigned int next_ext_offset = next_ext_hi << 16 + next_ext_lo;
  if (next_ext_offset)
    OLA_INFO << "Next Ext non-0, was " << next_ext_offset;

  if (!(*input >> packet->xid)) {
    OLA_INFO << packet_type << " too small to contain XID";
    return false;
  }

  if (!ExtractString(input, &packet->language, "Language"))
    return false;
  return true;
}


/**
 * Attempt to read a string from a data buffer. The first two bytes are the
 * string length. This un-escapes the string.
 */
bool SLPPacketParser::ExtractString(BigEndianInputStream *input,
                                    string *result,
                                    const string &field_name) const {
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

  // Unescape
  SLPStringUnescape(result);
  return true;
}


/**
 * Extract a list of strings, automatically un-escaping them
 */
bool SLPPacketParser::ExtractList(BigEndianInputStream *input,
                                  vector<string> *result,
                                  const string &field_name) const {
  uint16_t str_length;
  if (!(*input >> str_length)) {
    OLA_INFO << "Packet too small to read " << field_name << " length";
    return false;
  }

  string raw_string;
  unsigned int bytes_read = input->ReadString(&raw_string, str_length);
  if (bytes_read != str_length) {
    OLA_INFO << "Insufficent data remaining for SLP string " << field_name <<
      ", expected " << str_length << ", " << bytes_read << " remaining";
    return false;
  }

  StringSplit(raw_string, *result, ",");

  // Unescape all values
  vector<string>::iterator iter = result->begin();
  for (; iter != result->end(); ++iter)
    SLPStringUnescape(&(*iter));
  return true;
}


/**
 * Extract an URLEntry from the stream.
 */
bool SLPPacketParser::ExtractURLEntry(BigEndianInputStream *input,
                                      URLEntry *entry,
                                      const string &packet_type) const {
  uint8_t reserved;
  if (!ExtractValue(input, &reserved, packet_type + " reserved"))
    return false;

  uint16_t lifetime;
  if (!ExtractValue(input, &lifetime, packet_type + " lifetime"))
    return false;
  entry->Lifetime(lifetime);

  string url;
  if (!ExtractString(input, &url, packet_type + " URL"))
    return false;
  entry->URL(url);

  uint8_t url_auths;
  if (!ExtractValue(input, &url_auths, packet_type + " # of URL Auths"))
    return false;

  for (unsigned int i = 0; i < url_auths; i++) {
    if (!ExtractAuthBlock(input, packet_type))
      return false;
  }
  return true;
}


/**
 * Extract an Authentication block. We just discard these for now
 */
bool SLPPacketParser::ExtractAuthBlock(BigEndianInputStream *input,
                                       const string &packet_type) const {
  /*
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |  Block Structure Descriptor   |  Authentication Block Length  |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                           Timestamp                           |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     SLP SPI String Length     |         SLP SPI String        \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |              Structured Authentication Block ...              \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  uint16_t block_descriptor;
  if (!ExtractValue(input, &block_descriptor,
                    packet_type + " Auth block descriptor"))
    return false;

  uint16_t block_length;
  if (!ExtractValue(input, &block_length, packet_type + " Auth block length"))
    return false;

  uint32_t timestamp;
  if (!ExtractValue(input, &timestamp, packet_type + " Auth timestamp"))
    return false;

  string spi_string;
  if (!ExtractString(input, &spi_string, packet_type + "SPI String"))
    return false;

  int auth_block_size = block_length - (
      sizeof(block_descriptor) + sizeof(block_length) + sizeof(timestamp) +
      sizeof(block_length) + spi_string.size());

  if (auth_block_size < 0) {
    OLA_INFO << packet_type <<
      ": Auth block size smaller than the minimum value";
    return false;
  }

  if (auth_block_size == 0)
    return true;

  string auth_block_data;
  unsigned int bytes_read = input->ReadString(&auth_block_data,
                                              auth_block_size);
  if (bytes_read != static_cast<unsigned int>(auth_block_size)) {
    OLA_INFO << packet_type << ": insufficent data remaining for auth data";
    return false;
  }
  return true;
}


/**
 * Convert a comma separated string into a vector of IPAddresses.
 */
bool SLPPacketParser::ConvertIPAddressList(
    const string &list,
    vector<IPV4Address> *addresses) const {
  if (list.empty())
    return true;

  vector<string> str_list;
  StringSplit(list, str_list, ",");
  addresses->reserve(str_list.size());
  vector<string>::const_iterator iter = str_list.begin();
  bool ok = true;
  IPV4Address address;
  for (; iter != str_list.end(); ++iter) {
    if (IPV4Address::FromString(*iter, &address)) {
      addresses->push_back(address);
    } else {
      OLA_INFO << "SLP Packet contained invalid IP Address: " << *iter;
      ok = false;
    }
  }
  return ok;
}
}  // slp
}  // ola
