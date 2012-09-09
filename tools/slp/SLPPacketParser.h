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

#include <ola/network/IPV4Address.h>

#include <string>
#include <vector>

#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/URLEntry.h"

using ola::io::IOQueue;
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

    // members
    xid_t xid;
    uint16_t flags;
    string language;

    bool Overflow() const { return flags & SLP_OVERFLOW; }
    bool Fresh() const { return flags & SLP_FRESH; }
    bool Multicast() const { return flags & SLP_REQUEST_MCAST; }
};


/*
 * A Service Request message
 */
class ServiceRequestPacket: public SLPPacket {
  public:
    ServiceRequestPacket()
        : SLPPacket() {
    }

    vector<IPV4Address> pr_list;
    string service_type;
    vector<string> scope_list;
};


/**
 * The SLPPacketParser unpacks data from SLP packets.
 */
class SLPPacketParser {
  public:
    SLPPacketParser() {}
    ~SLPPacketParser() {}

    // Return the function-id for a packet, or 0 if the packet is malformed
    uint8_t DetermineFunctionID(const uint8_t *data,
                                unsigned int length) const;

    const ServiceRequestPacket* UnpackServiceRequest(
        const uint8_t *data,
        unsigned int length) const;

    //UnpackServiceRequest(const uint8_t *data, unsigned int length);

    //UnpackService

  private:
    // unpack header
    bool ExtractHeader(const uint8_t *data,
                       unsigned int length,
                       SLPPacket *packet,
                       unsigned int *data_offset) const;
    bool ReadString(uint8_t *data, unsigned int length,
                    const string &field_name, string *result) const;
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPPACKETPARSER_H_
