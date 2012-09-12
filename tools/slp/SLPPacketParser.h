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
 * SLPPacketParser.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPPACKETPARSER_H_
#define TOOLS_SLP_SLPPACKETPARSER_H_

#include <ola/Logging.h>
#include <ola/io/BigEndianStream.h>
#include <ola/network/IPV4Address.h>

#include <string>
#include <vector>

#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/URLEntry.h"

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
    vector<string> scope_list;
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
 * The SLPPacketParser unpacks data from SLP packets.
 */
class SLPPacketParser {
  public:
    SLPPacketParser() {}
    ~SLPPacketParser() {}

    uint8_t DetermineFunctionID(const uint8_t *data,
                                unsigned int length) const;

    const ServiceRequestPacket* UnpackServiceRequest(
        BigEndianInputStream *input) const;

    const ServiceReplyPacket* UnpackServiceReply(
        BigEndianInputStream *input) const;

  private:
    bool ExtractHeader(BigEndianInputStream *input,
                       SLPPacket *packet,
                       const string &packet_type) const;

    bool ExtractString(BigEndianInputStream *input,
                       string *result,
                       const string &field_name) const;

    bool ExtractURLEntry(BigEndianInputStream *input,
                         URLEntry *entry,
                         const string &packet_type) const;

    bool ExtractAuthBlock(BigEndianInputStream *input,
                          const string &packet_type) const;

    bool ConvertIPAddressList(const string &list,
                              vector<IPV4Address> *addresses) const;

    template <typename T>
    bool ExtractValue(BigEndianInputStream *stream,
                      T *value,
                      const string &field_name) const {
      if ((*stream >> *value))
        return true;
      OLA_INFO << "Packet too small to contain " << field_name;
      return false;
    }
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPPACKETPARSER_H_
