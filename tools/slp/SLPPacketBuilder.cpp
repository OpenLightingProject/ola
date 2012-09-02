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
 * SLPPacketBuilder.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketConstants.h"

#include <string>
#include <vector>

namespace ola {
namespace slp {

using ola::io::IOQueue;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using std::string;
using std::vector;


/**
 * Build an DAAdvert Packet
 * @param output the IOQueue to put the packet in
 * @param error_code, should be 0 if this packet will be multicast
 */
void SLPPacketBuilder::BuildDAAdvert(IOQueue *output,
                                     xid_t xid,
                                     bool multicast,
                                     uint16_t error_code,
                                     uint32_t boot_timestamp,
                                     const string &url,
                                     const string &scope_list) {
  /*
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |        Service Location header (function = DAAdvert = 8)      |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |          Error Code           |  DA Stateless Boot Timestamp  |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |DA Stateless Boot Time,, contd.|         Length of URL         |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     \                              URL                              \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     Length of <scope-list>    |         <scope-list>          \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     Length of <attr-list>     |          <attr-list>          \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |    Length of <SLP SPI List>   |     <SLP SPI List> String     \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     | # Auth Blocks |         Authentication block (if any)         \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  unsigned int length = 8 + url.size() + 7;
  BuildSLPHeader(output,
                 DA_ADVERTISEMENT,
                 length,
                 multicast ? SLP_REQUEST_MCAST : 0,
                 xid);

  *output << HostToNetwork(static_cast<uint16_t>(multicast ? 0 : error_code));
  *output << HostToNetwork(boot_timestamp);
  *output << HostToNetwork(static_cast<uint16_t>(url.size()));
  output->Write(reinterpret_cast<const uint8_t*>(url.data()),
                url.size());
  *output << HostToNetwork(static_cast<uint16_t>(scope_list.size()));
  output->Write(reinterpret_cast<const uint8_t*>(scope_list.data()),
                scope_list.size());
  *output << static_cast<uint16_t>(0);  // length of attr-list
  *output << static_cast<uint16_t>(0);  // length of spi list
  *output << static_cast<uint8_t>(0);   // # of auth blocks
}



/**
 * Build the SLP header in an IOQueue.
 * @param output the IOQueue to build the packet in
 * @param function_id the Function-ID for this packet
 * @param length the length in the contents after the header
 * @param xid the xid (transaction #) for the packet
 */
void SLPPacketBuilder::BuildSLPHeader(IOQueue *output,
                                      slp_function_id_t function_id,
                                      unsigned int length,
                                      uint8_t flags,
                                      xid_t xid) {
  if (!output->Empty())
    OLA_WARN << "Building SLP header in non-empty IOQueue!";

  length += 16;
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
  *output << SLP_VERSION << static_cast<uint8_t>(function_id) <<
    static_cast<uint16_t>(length >> 8);
  *output << HostToNetwork(static_cast<uint8_t>(length)) << flags <<
    static_cast<uint16_t>(0);
  *output << static_cast<uint16_t>(0) << HostToNetwork(xid);
  *output << HostToNetwork(static_cast<uint16_t>(sizeof(EN_LANGUAGE_TAG)));
  output->Write(EN_LANGUAGE_TAG, sizeof(EN_LANGUAGE_TAG));
}
}  // slp
}  // ola
