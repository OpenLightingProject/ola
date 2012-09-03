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
    uint8_t Version() const { return m_version; }
    uint8_t FunctionID() const { return m_function_id; }
    unsigned int Length() const { return m_length; }

    bool Overflow() const { return m_flags & SLP_OVERFLOW; }
    bool Fresh() const { return m_flags & SLP_FRESH; }
    bool Multicast() const { return m_flags & SLP_REQUEST_MCAST; }
    unsigned int NextExtOffset() const { return m_next_ext_offset; }
    string Language() const { return m_lang; }

  private:
    uint8_t m_version;
    uint8_t m_function_id;
    unsigned int m_length;
    uint16_t m_flags;
    unsigned int m_next_ext_offset;
    string m_lang;

    SLPPacket() {}
    virtual ~SLPPacket() {}
};


/*
 * A Service Request message
 */
class ServiceRequestPacket: public SLPPacket {
  public:




  private:



};


/**
 * The SLPPacketParser unpacks data from SLP packets.
 */
class SLPPacketParser {
  public:
    SLPPacketParser() {}
    ~SLPPacketParser() {}

    // Return the function-id for a packet, or 0 if the packet is malformed
    static uint8_t DetermineFunctionID(const uint8_t *data,
                                       unsigned int length);

    //UnpackServiceRequest(const uint8_t *data, unsigned int length);

    //UnpackService

  private:
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPPACKETPARSER_H_
