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
 * SLPPacketParser.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef SLP_SLPPACKETPARSER_H_
#define SLP_SLPPACKETPARSER_H_

#include <ola/Logging.h>
#include <ola/io/BigEndianStream.h>
#include <ola/network/IPV4Address.h>

#include <string>
#include <vector>

#include "slp/SLPPacketConstants.h"
#include "slp/ServiceEntry.h"

using ola::io::BigEndianInputStream;
using ola::network::IPV4Address;
using std::string;
using std::vector;

namespace ola {
namespace slp {

/**
 * The base SLP Packet class.
 */
class SLPPacket {
  public:
    SLPPacket()
      : xid(0),
        flags(0),
        language("") {
    }
    virtual ~SLPPacket() {}

    bool Overflow() const { return flags & SLP_OVERFLOW; }
    bool Fresh() const { return flags & SLP_FRESH; }
    bool Multicast() const { return flags & SLP_REQUEST_MCAST; }

    // members
    xid_t xid;
    uint16_t flags;
    string language;
};


/*
 * A Service Request message
 */
class ServiceRequestPacket: public SLPPacket {
  public:
    vector<IPV4Address> pr_list;
    string service_type;
    string scope_list;
    string predicate;
    string spi;
};


/**
 * A Service Reply
 */
class ServiceReplyPacket: public SLPPacket {
  public:
    ServiceReplyPacket(): SLPPacket(), error_code(0) {}

    uint16_t error_code;
    vector<URLEntry> url_entries;
};


/**
 * A Service Registration
 */
class ServiceRegistrationPacket: public SLPPacket {
  public:
    ServiceRegistrationPacket(): SLPPacket() {}

    URLEntry url;
    string service_type;
    string scope_list;
    string attr_list;
};


/**
 * A Service De-Registration
 */
class ServiceDeRegistrationPacket: public SLPPacket {
  public:
    ServiceDeRegistrationPacket(): SLPPacket() {}

    URLEntry url;
    string scope_list;
    string tag_list;
};


/**
 * A Service Ack
 */
class ServiceAckPacket: public SLPPacket {
  public:
    ServiceAckPacket(): SLPPacket() {}

    uint16_t error_code;
};


/**
 * A DA Advert
 */
class DAAdvertPacket: public SLPPacket {
  public:
    DAAdvertPacket(): SLPPacket() {}

    uint16_t error_code;
    uint32_t boot_timestamp;
    string url;
    string scope_list;
    string attr_list;
    string spi_string;
};


/**
 * SrvTypeRqst
 */
class ServiceTypeRequestPacket: public SLPPacket {
  public:
    ServiceTypeRequestPacket()
        : SLPPacket(),
          include_all(false) {
    }

    vector<IPV4Address> pr_list;
    bool include_all;
    string naming_authority;
    string scope_list;
};


/**
 * The SLPPacketParser unpacks data from SLP packets.
 */
class SLPPacketParser {
  public:
    SLPPacketParser() {}
    ~SLPPacketParser() {}

    static uint8_t DetermineFunctionID(const uint8_t *data,
                                       unsigned int length);

    static const ServiceRequestPacket *UnpackServiceRequest(
        BigEndianInputStream *input);

    static const ServiceReplyPacket *UnpackServiceReply(
        BigEndianInputStream *input);

    static const ServiceRegistrationPacket *UnpackServiceRegistration(
        BigEndianInputStream *input);

    static const ServiceAckPacket *UnpackServiceAck(
        BigEndianInputStream *input);

    static const DAAdvertPacket *UnpackDAAdvert(
        BigEndianInputStream *input);

    static const ServiceTypeRequestPacket *UnpackServiceTypeRequest(
        BigEndianInputStream *input);

    static const ServiceDeRegistrationPacket *UnpackServiceDeRegistration(
        BigEndianInputStream *input);

    static bool ExtractHeader(BigEndianInputStream *input,
                              SLPPacket *packet,
                              const string &packet_type);

  private:
    static bool ExtractString(BigEndianInputStream *input,
                              string *result,
                              const string &field_name,
                              bool unescape = true);

    static bool ExtractList(BigEndianInputStream *input,
                            vector<string> *result,
                            const string &field_name);

    static bool ExtractURLEntry(BigEndianInputStream *input,
                                URLEntry *entry,
                                const string &packet_type);

    static bool ExtractAuthBlock(BigEndianInputStream *input,
                                 const string &packet_type);

    static void ConvertIPAddressList(const string &list,
                                     vector<IPV4Address> *addresses);

    template <typename T>
    static bool ExtractValue(BigEndianInputStream *stream,
                      T *value,
                      const string &field_name) {
      if ((*stream >> *value))
        return true;
      OLA_INFO << "Packet too small to contain " << field_name;
      return false;
    }
};
}  // namespace slp
}  // namespace ola
#endif  // SLP_SLPPACKETPARSER_H_
