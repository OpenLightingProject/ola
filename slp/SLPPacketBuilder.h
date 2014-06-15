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
 * SLPPacketBuilder.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef SLP_SLPPACKETBUILDER_H_
#define SLP_SLPPACKETBUILDER_H_

#include <ola/io/BigEndianStream.h>
#include <ola/network/IPV4Address.h>

#include <set>
#include <string>
#include <vector>

#include "slp/SLPPacketConstants.h"
#include "slp/ServiceEntry.h"
#include "slp/ScopeSet.h"

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
                                    const string &language,
                                    const set<IPV4Address> &pr_list,
                                    const string &service_type,
                                    const ScopeSet &scopes,
                                    const string &predicate = "");

    static void BuildServiceRequest(BigEndianOutputStreamInterface *output,
                                    xid_t xid,
                                    bool multicast,
                                    const string &language,
                                    const string &pr_list,
                                    const string &service_type,
                                    const ScopeSet &scopes,
                                    const string &predicate);

    static void BuildServiceReply(BigEndianOutputStreamInterface *output,
                                  xid_t xid,
                                  const string &language,
                                  uint16_t error_code,
                                  const URLEntries &urls);

    static void BuildServiceRegistration(BigEndianOutputStreamInterface *output,
                                         xid_t xid,
                                         bool fresh,
                                         const ScopeSet &scopes,
                                         const ServiceEntry &service);

    static void BuildServiceDeRegistration(
        BigEndianOutputStreamInterface *output,
        xid_t xid,
        const ScopeSet &scopes,
        const ServiceEntry &service);

    static void BuildServiceAck(BigEndianOutputStreamInterface *output,
                                xid_t xid,
                                const string &language,
                                uint16_t error_code);

    static void BuildDAAdvert(BigEndianOutputStreamInterface *output,
                              xid_t xid,
                              bool multicast,
                              uint16_t error_code,
                              uint32_t boot_timestamp,
                              const string &url,
                              const ScopeSet &scopes);

    static void BuildAllServiceTypeRequest(
        BigEndianOutputStreamInterface *output,
        xid_t xid,
        bool multicast,
        const set<IPV4Address> &pr_list,
        const ScopeSet &scopes);
    static void BuildServiceTypeRequest(BigEndianOutputStreamInterface *output,
                                        xid_t xid,
                                        bool multicast,
                                        const set<IPV4Address> &pr_list,
                                        const string &naming_auth,
                                        const ScopeSet &scopes);

    static void BuildServiceTypeReply(BigEndianOutputStreamInterface *output,
                                      xid_t xid,
                                      uint16_t error_code,
                                      const vector<string> &service_types);

    static void BuildSAAdvert(BigEndianOutputStreamInterface *output,
                              xid_t xid,
                              bool multicast,
                              const string &url,
                              const ScopeSet &scopes);

    static void BuildError(BigEndianOutputStreamInterface *output,
                           slp_function_id_t function_id,
                           xid_t xid,
                           const string &language,
                           uint16_t error_code);

    static void WriteString(BigEndianOutputStreamInterface *output,
                            const string &data);

    static void BuildSLPHeader(BigEndianOutputStreamInterface *output,
                               slp_function_id_t function_id,
                               unsigned int length,
                               uint16_t flags,
                               xid_t xid,
                               const string &language);
};
}  // namespace slp
}  // namespace ola
#endif  // SLP_SLPPACKETBUILDER_H_
