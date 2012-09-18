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
 * SLPPacketBuilder.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPPACKETBUILDER_H_
#define TOOLS_SLP_SLPPACKETBUILDER_H_

#include <ola/io/BigEndianStream.h>
#include <ola/network/IPV4Address.h>

#include <set>
#include <string>

#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/URLEntry.h"

using ola::io::BigEndianOutputStreamAdaptor;
using ola::io::BigEndianOutputStreamInterface;
using ola::network::IPV4Address;
using std::set;
using std::string;

namespace ola {
namespace slp {

/**
 * The SLPPacketBuilder constructs packets.
 */
class SLPPacketBuilder {
  public:
    SLPPacketBuilder() {}
    ~SLPPacketBuilder() {}

    static void BuildServiceRequest(BigEndianOutputStreamInterface *output,
                                    xid_t xid,
                                    bool multicast,
                                    const set<IPV4Address> &pr_list,
                                    const string &service_type,
                                    const set<string> &scope_list);

    static void BuildServiceReply(BigEndianOutputStreamInterface *output,
                                  xid_t xid,
                                  uint16_t error_code,
                                  const URLEntries &url_entries);

    static void BuildServiceRegistration(BigEndianOutputStreamInterface *output,
                                         xid_t xid,
                                         bool fresh,
                                         const URLEntry &url_entry,
                                         const string &service_type,
                                         set<string> &scope_list);

    static void BuildServiceAck(BigEndianOutputStreamInterface *output,
                                xid_t xid,
                                uint16_t error_code);

    static void BuildDAAdvert(BigEndianOutputStreamInterface *output,
                              xid_t xid,
                              bool multicast,
                              uint16_t error_code,
                              uint32_t boot_timestamp,
                              const string &url,
                              const set<string> &scope_list);

    static void WriteString(BigEndianOutputStreamInterface *output,
                            const string &data);

  private:
    static void BuildSLPHeader(BigEndianOutputStreamInterface *output,
                               slp_function_id_t function_id,
                               unsigned int length,
                               uint16_t flags,
                               xid_t xid);

    static void EscapeAndJoin(const set<string> &list, string *output);
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPPACKETBUILDER_H_
