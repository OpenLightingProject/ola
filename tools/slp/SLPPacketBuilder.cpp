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
#include <ola/StringUtils.h>
#include <ola/io/BigEndianStream.h>
#include <ola/network/NetworkUtils.h>

#include <set>
#include <string>

#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/SLPStrings.h"
#include "tools/slp/URLEntry.h"

namespace ola {
namespace slp {

using ola::io::BigEndianOutputStreamAdaptor;
using ola::StringJoin;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using std::string;
using std::set;


/**
 * Build a Service Request.
 * @param output the OutputStreamInterface to put the packet in
 * @param xid the transaction ID
 * @param pr_list the previous responder ist
 * @param service_type the service to locate
 * @param scope_list the list of scopes to search.
 */
void SLPPacketBuilder::BuildServiceRequest(
    BigEndianOutputStreamInterface *output,
    xid_t xid,
    bool multicast,
    const set<IPV4Address> &pr_list,
    const string &service_type,
    const set<string> &scope_list) {
  /*
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |       Service Location header (function = SrvRqst = 1)        |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |      length of <PRList>       |        <PRList> String        \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |   length of <service-type>    |    <service-type> String      \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |    length of <scope-list>     |     <scope-list> String       \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |  length of predicate string   |  Service Request <predicate>  \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |  length of <SLP SPI> string   |       <SLP SPI> String        \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  const string joined_pr_list = ola::StringJoin(",", pr_list);
  string joined_scopes;
  EscapeAndJoin(scope_list, &joined_scopes);

  unsigned int length = (10 + joined_pr_list.size() + service_type.size() +
                         joined_scopes.size());
  BuildSLPHeader(output,
                 SERVICE_REQUEST,
                 length,
                 multicast ? SLP_REQUEST_MCAST : 0,
                 xid);
  WriteString(output, joined_pr_list);
  WriteString(output, service_type);
  WriteString(output, joined_scopes);
  *output << static_cast<uint16_t>(0);   // length of predicate string
  *output << static_cast<uint16_t>(0);   // length of SPI
}


/**
 * Build a Service Reply.
 * @param output the BigEndianOutputStreamInterface to put the packet in
 * @param xid the transaction ID
 * @param error_code the SLP error code
 * @param url_entries a list of URLEntries to return.
 */
void SLPPacketBuilder::BuildServiceReply(BigEndianOutputStreamInterface *output,
                                         xid_t xid,
                                         uint16_t error_code,
                                         const URLEntries &url_entries) {
  /*
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |        Service Location header (function = SrvRply = 2)       |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |        Error Code             |        URL Entry count        |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |       <URL Entry 1>          ...       <URL Entry N>          \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  unsigned int length = 4;
  URLEntries::const_iterator iter = url_entries.begin();
  for (; iter != url_entries.end(); ++iter)
    length += iter->Size();

  BuildSLPHeader(output, SERVICE_REPLY, length, 0, xid);
  *output << error_code;
  *output << static_cast<uint16_t>(url_entries.size());

  for (iter = url_entries.begin(); iter != url_entries.end(); ++iter)
    iter->Write(output);
}


/**
 * Build a Service Registration message.
 * @param output the BigEndianOutputStreamInterface to put the packet in
 * @param xid the transaction ID
 * @param fresh set to true if this is a new registration.
 * @param url_entry the URLEntry to include
 * @param service_type the SLP service-type
 * @param scope_list a list of scopes.
 */
void SLPPacketBuilder::BuildServiceRegistration(
    BigEndianOutputStreamInterface *output,
    xid_t xid,
    bool fresh,
    const URLEntry &url_entry,
    const string &service_type,
    set<string> &scope_list) {
  /*
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |         Service Location header (function = SrvReg = 3)       |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                          <URL-Entry>                          \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     | length of service type string |        <service-type>         \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     length of <scope-list>    |         <scope-list>          \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |  length of attr-list string   |          <attr-list>          \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |# of AttrAuths |(if present) Attribute Authentication Blocks...\
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  string joined_scopes;
  EscapeAndJoin(scope_list, &joined_scopes);
  unsigned int length = (url_entry.Size() + 2 + service_type.size() +
                         2 + joined_scopes.size() + 3);

  BuildSLPHeader(output, SERVICE_REGISTRATION, length, fresh ? SLP_FRESH : 0,
                 xid);
  url_entry.Write(output);
  WriteString(output, service_type);
  WriteString(output, joined_scopes);
  *output << static_cast<uint16_t>(0);  // length of attr-list
  *output << static_cast<uint8_t>(0);   // # of AttrAuths
}


/**
 * Build a Service Registration message.
 * @param output the BigEndianOutputStreamInterface to put the packet in
 * @param xid the transaction ID
 * @param error_code.
 */
void SLPPacketBuilder::BuildServiceAck(BigEndianOutputStreamInterface *output,
                                       xid_t xid,
                                       uint16_t error_code) {
  /*
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |          Service Location header (function = SrvAck = 5)      |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |          Error Code           |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  BuildSLPHeader(output, SERVICE_ACKNOWLEDGE, 2, 0, xid);
  *output << error_code;
}


/**
 * Build an DAAdvert Packet
 * @param output the BigEndianOutputStreamInterface to put the packet in
 * @param xid the transaction ID
 * @param multicast true if this packet will be multicast
 * @param error_code, should be 0 if this packet will be multicast
 * @param boot_timestamp the boot time of the DA
 * @param url the URL to use.
 * @param scope_list a list of scopes.
 */
void SLPPacketBuilder::BuildDAAdvert(BigEndianOutputStreamInterface *output,
                                     xid_t xid,
                                     bool multicast,
                                     uint16_t error_code,
                                     uint32_t boot_timestamp,
                                     const string &url,
                                     const set<string> &scope_list) {
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
  string joined_scopes;
  EscapeAndJoin(scope_list, &joined_scopes);
  unsigned int length = 8 + url.size() + + joined_scopes.size() + 7;
  BuildSLPHeader(output,
                 DA_ADVERTISEMENT,
                 length,
                 multicast ? SLP_REQUEST_MCAST : 0,
                 xid);

  *output << static_cast<uint16_t>(multicast ? 0 : error_code);
  *output << boot_timestamp;
  WriteString(output, url);
  WriteString(output, joined_scopes);
  *output << static_cast<uint16_t>(0);  // length of attr-list
  *output << static_cast<uint16_t>(0);  // length of spi list
  *output << static_cast<uint8_t>(0);   // # of auth blocks
}


/**
 * Build an SAAdvert Packet
 * @param output the BigEndianOutputStreamInterface to put the packet in
 * @param xid the transaction ID
 * @param multicast true if this packet will be multicast
 * @param url the URL to use.
 * @param scope_list a list of scopes.
 */
void SLPPacketBuilder::BuildSAAdvert(BigEndianOutputStreamInterface *output,
                                     xid_t xid,
                                     bool multicast,
                                     const string &url,
                                     const set<string> &scope_list) {
  /*
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |        Service Location header (function = SAAdvert = 11)     |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |         Length of URL         |              URL              \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     Length of <scope-list>    |         <scope-list>          \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     Length of <attr-list>     |          <attr-list>          \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     | # auth blocks |        authentication block (if any)          \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  string joined_scopes;
  EscapeAndJoin(scope_list, &joined_scopes);
  unsigned int length = 7 + url.size() + + joined_scopes.size();
  BuildSLPHeader(output,
                 SA_ADVERTISEMENT,
                 length,
                 multicast ? SLP_REQUEST_MCAST : 0,
                 xid);

  WriteString(output, url);
  WriteString(output, joined_scopes);
  *output << static_cast<uint16_t>(0);  // length of attr-list
  *output << static_cast<uint8_t>(0);   // # of auth blocks
}


/**
 * Write a string to an OutputStreamInterface. The length of the string is
 * written in network-byte order as the first two bytes.
 */
void SLPPacketBuilder::WriteString(BigEndianOutputStreamInterface *output,
                                   const string &data) {
  *output << static_cast<uint16_t>(data.size());
  output->Write(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}


/**
 * Build the SLP header in an OutputStreamInterface.
 * @param output the OutputStreamInterface to build the packet in
 * @param function_id the Function-ID for this packet
 * @param length the length in the contents after the header
 * @param xid the xid (transaction #) for the packet
 */
void SLPPacketBuilder::BuildSLPHeader(BigEndianOutputStreamInterface *output,
                                      slp_function_id_t function_id,
                                      unsigned int length,
                                      uint16_t flags,
                                      xid_t xid) {
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
  *output << static_cast<uint8_t>(length) << flags << static_cast<uint8_t>(0);
  *output << static_cast<uint16_t>(0) << xid;
  *output << static_cast<uint16_t>(sizeof(EN_LANGUAGE_TAG));
  output->Write(EN_LANGUAGE_TAG, sizeof(EN_LANGUAGE_TAG));
}


/**
 * Join a set of strings after escaping each string.
 */
void SLPPacketBuilder::EscapeAndJoin(const set<string> &list, string *output) {
  std::ostringstream str;
  set<string>::const_iterator iter = list.begin();
  while (iter != list.end()) {
    string val = *iter;
    SLPStringEscape(&val);
    output->append(val);
    iter++;
    if (iter != list.end())
      output->append(",");
  }
}
}  // slp
}  // ola
