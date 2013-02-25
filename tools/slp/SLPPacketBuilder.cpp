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
 * SLPPacketBuilder.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/io/BigEndianStream.h>
#include <ola/network/NetworkUtils.h>

#include <set>
#include <string>
#include <vector>

#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/SLPStrings.h"
#include "tools/slp/ServiceEntry.h"

namespace ola {
namespace slp {

using ola::io::BigEndianOutputStreamAdaptor;
using ola::StringJoin;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using std::set;
using std::string;


/**
 * Build a Service Request.
 * @param output the OutputStreamInterface to put the packet in
 * @param xid the transaction ID
 * @param pr_list the previous responder ist
 * @param service_type the service to locate
 * @param scopes the set of scopes to search.
 */
void SLPPacketBuilder::BuildServiceRequest(
    BigEndianOutputStreamInterface *output,
    xid_t xid,
    bool multicast,
    const string &language,
    const set<IPV4Address> &pr_list,
    const string &service_type,
    const ScopeSet &scopes,
    const string &predicate) {
  const string joined_pr_list = ola::StringJoin(",", pr_list);
  BuildServiceRequest(output, xid, multicast, language, joined_pr_list,
                      service_type, scopes, predicate);
}


/**
 * Similar to above but allows predicates and non-IPv4 addresses in the PR list
 */
void SLPPacketBuilder::BuildServiceRequest(
    BigEndianOutputStreamInterface *output,
    xid_t xid,
    bool multicast,
    const string &language,
    const string &pr_list,
    const string &service_type,
    const ScopeSet &scopes,
    const string &predicate) {
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
  const string joined_scopes = scopes.AsEscapedString();
  unsigned int length = (10 + pr_list.size() + service_type.size() +
                         joined_scopes.size());
  BuildSLPHeader(output, SERVICE_REQUEST, length,
                 multicast ? SLP_REQUEST_MCAST : 0, xid, language);
  WriteString(output, pr_list);
  WriteString(output, service_type);
  WriteString(output, joined_scopes);
  WriteString(output, predicate);
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
                                         const string &language,
                                         uint16_t error_code,
                                         const URLEntries &urls) {
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
  URLEntries::const_iterator iter;
  for (iter = urls.begin(); iter != urls.end(); ++iter)
    length += iter->PackedSize();

  BuildSLPHeader(output, SERVICE_REPLY, length, 0, xid, language);
  *output << error_code;
  *output << static_cast<uint16_t>(urls.size());

  for (iter = urls.begin(); iter != urls.end(); ++iter)
    iter->Write(output);
}


/**
 * Build a Service Registration message.
 * @param output the BigEndianOutputStreamInterface to put the packet in
 * @param xid the transaction ID
 * @param fresh set to true if this is a new registration.
 * @param scopes the set of scopes to use. We don't use the ones from the
 *   ServiceEntry, since the DA may not support all of them, see Section 8.3
 * @param service the ServiceEntry to register
 */
void SLPPacketBuilder::BuildServiceRegistration(
    BigEndianOutputStreamInterface *output,
    xid_t xid,
    bool fresh,
    const ScopeSet &scopes,
    const ServiceEntry &service) {
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
  const string joined_scopes = scopes.AsEscapedString();
  unsigned int length = (service.url().PackedSize() + 2 +
                         service.service_type().size() +
                         2 + joined_scopes.size() + 3);

  BuildSLPHeader(output, SERVICE_REGISTRATION, length, fresh ? SLP_FRESH : 0,
                 xid, EN_LANGUAGE_TAG);
  service.url().Write(output);
  WriteString(output, service.service_type());
  WriteString(output, joined_scopes);
  *output << static_cast<uint16_t>(0);  // length of attr-list
  *output << static_cast<uint8_t>(0);   // # of AttrAuths
}


/**
 * Build a Service De-Registration message.
 * @param output the BigEndianOutputStreamInterface to put the packet in
 * @param xid the transaction ID
 * @param fresh set to true if this is a new registration.
 * @param scopes the set of scopes to use. We don't use the ones from the
 *   ServiceEntry, since the DA may not support all of them, see Section 8.3
 * @param service the ServiceEntry to register
 */
void SLPPacketBuilder::BuildServiceDeRegistration(
    BigEndianOutputStreamInterface *output,
    xid_t xid,
    const ScopeSet &scopes,
    const ServiceEntry &service) {
  /*
      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |         Service Location header (function = SrvDeReg = 4)     |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |    Length of <scope-list>     |         <scope-list>          \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                           URL Entry                           \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |      Length of <tag-list>     |            <tag-list>         \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  const string joined_scopes = scopes.AsEscapedString();
  unsigned int length = (2 + joined_scopes.size() +
                         service.url().PackedSize() + 2);

  BuildSLPHeader(output, SERVICE_DEREGISTER, length, 0, xid, EN_LANGUAGE_TAG);
  WriteString(output, joined_scopes);
  service.url().Write(output);
  *output << static_cast<uint16_t>(0);  // length of tag-list
}


/**
 * Build a Service Registration message.
 * @param output the BigEndianOutputStreamInterface to put the packet in
 * @param xid the transaction ID
 * @param error_code.
 */
void SLPPacketBuilder::BuildServiceAck(BigEndianOutputStreamInterface *output,
                                       xid_t xid,
                                       const string &language,
                                       uint16_t error_code) {
  /*
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |          Service Location header (function = SrvAck = 5)      |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |          Error Code           |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  BuildSLPHeader(output, SERVICE_ACKNOWLEDGE, 2, 0, xid, language);
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
                                     const ScopeSet &scopes) {
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
  const string joined_scopes = scopes.AsEscapedString();
  unsigned int length = 8 + url.size() + + joined_scopes.size() + 7;
  BuildSLPHeader(output,
                 DA_ADVERTISEMENT,
                 length,
                 multicast ? SLP_REQUEST_MCAST : 0,
                 xid,
                 EN_LANGUAGE_TAG);

  *output << static_cast<uint16_t>(multicast ? 0 : error_code);
  *output << boot_timestamp;
  WriteString(output, url);
  WriteString(output, joined_scopes);
  *output << static_cast<uint16_t>(0);  // length of attr-list
  *output << static_cast<uint16_t>(0);  // length of spi list
  *output << static_cast<uint8_t>(0);   // # of auth blocks
}


/**
 * Build a request for all service types
 */
void SLPPacketBuilder::BuildAllServiceTypeRequest(
    BigEndianOutputStreamInterface *output,
    xid_t xid,
    bool multicast,
    const set<IPV4Address> &pr_list,
    const ScopeSet &scopes) {
  /*
      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |      Service Location header (function = SrvTypeRqst = 9)     |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |        length of PRList       |        <PRList> String        \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |   length of Naming Authority  |   <Naming Authority String>   \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     length of <scope-list>    |      <scope-list> String      \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  const string joined_pr_list = ola::StringJoin(",", pr_list);
  const string joined_scopes = scopes.AsEscapedString();
  unsigned int length = 6 + joined_pr_list.size() + joined_scopes.size();
  BuildSLPHeader(output,
                 SERVICE_TYPE_REQUEST,
                 length,
                 multicast ? SLP_REQUEST_MCAST : 0,
                 xid, EN_LANGUAGE_TAG);

  WriteString(output, joined_pr_list);
  *output << static_cast<uint16_t>(0xffff);  // All services
  WriteString(output, joined_scopes);
}


/**
 * Build a service-type request for a specific naming auth
 */
void SLPPacketBuilder::BuildServiceTypeRequest(
    BigEndianOutputStreamInterface *output,
    xid_t xid,
    bool multicast,
    const set<IPV4Address> &pr_list,
    const string &naming_auth,
    const ScopeSet &scopes) {
  /*
      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |      Service Location header (function = SrvTypeRqst = 9)     |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |        length of PRList       |        <PRList> String        \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |   length of Naming Authority  |   <Naming Authority String>   \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     length of <scope-list>    |      <scope-list> String      \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */
  const string joined_pr_list = ola::StringJoin(",", pr_list);
  const string joined_scopes = scopes.AsEscapedString();
  unsigned int length = 6 + joined_pr_list.size() + naming_auth.size() +
                        joined_scopes.size();
  BuildSLPHeader(output,
                 SERVICE_TYPE_REQUEST,
                 length,
                 multicast ? SLP_REQUEST_MCAST : 0,
                 xid, EN_LANGUAGE_TAG);

  WriteString(output, joined_pr_list);
  WriteString(output, naming_auth);
  WriteString(output, joined_scopes);
}


/**
 * Build a SrvTypeRply packet
 */
void SLPPacketBuilder::BuildServiceTypeReply(
    BigEndianOutputStreamInterface *output,
    xid_t xid,
    uint16_t error_code,
    const vector<string> &service_types) {
  /*
      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |      Service Location header (function = SrvTypeRply = 10)    |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |           Error Code          |    length of <srvType-list>   |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                       <srvtype--list>                         \
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
  vector<string> escaped_service_types;
  vector<string>::const_iterator iter = service_types.begin();
  for (; iter != service_types.end(); ++iter) {
    string service_type = *iter;
    SLPStringEscape(&service_type);
    escaped_service_types.push_back(service_type);
  }
  const string joined_service_types = ola::StringJoin(
      ",", escaped_service_types);
  unsigned int length = 4 + joined_service_types.size();
  BuildSLPHeader(output, SERVICE_TYPE_REPLY, length, false, xid,
                 EN_LANGUAGE_TAG);

  *output << static_cast<uint16_t>(error_code);
  WriteString(output, joined_service_types);
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
                                     const ScopeSet &scopes) {
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
  const string joined_scopes = scopes.AsEscapedString();
  unsigned int length = 7 + url.size() + + joined_scopes.size();
  BuildSLPHeader(output,
                 SA_ADVERTISEMENT,
                 length,
                 multicast ? SLP_REQUEST_MCAST : 0,
                 xid, EN_LANGUAGE_TAG);

  WriteString(output, url);
  WriteString(output, joined_scopes);
  *output << static_cast<uint16_t>(0);  // length of attr-list
  *output << static_cast<uint8_t>(0);   // # of auth blocks
}


/**
 * Build an error message. Error messages consist of the SLP Header, plus the 2
 * byte error code. They are actually truncated SrvRply or DAAdvert messages.
 */
void SLPPacketBuilder::BuildError(BigEndianOutputStreamInterface *output,
                                  slp_function_id_t function_id,
                                  xid_t xid,
                                  const string &language,
                                  uint16_t error_code) {
  BuildSLPHeader(output, function_id, 2, false, xid, language);
  *output << static_cast<uint16_t>(error_code);
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
                                      xid_t xid,
                                      const string &language) {
  length += (14 + language.size());
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
  *output << SLP_VERSION << static_cast<uint8_t>(function_id);
  *output << static_cast<uint16_t>(length >> 8);
  *output << static_cast<uint8_t>(length) << flags << static_cast<uint8_t>(0);
  *output << static_cast<uint16_t>(0) << xid;
  WriteString(output, language);
}
}  // slp
}  // ola
