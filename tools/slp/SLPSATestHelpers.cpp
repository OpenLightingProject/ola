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
 * SLPSATestHelpers.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/io/BigEndianStream.h>
#include <ola/io/MemoryBuffer.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/UID.h>
#include <memory>
#include <set>
#include <string>

#include "tools/e133/E133URLParser.h"
#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/SLPSATestRunner.h"
#include "tools/slp/SLPStrings.h"
#include "tools/slp/ScopeSet.h"
#include "tools/slp/URLEntry.h"

using ola::io::BigEndianOutputStream;
using ola::network::IPV4Address;
using ola::rdm::UID;
using ola::slp::EN_LANGUAGE_TAG;
using ola::slp::SERVICE_REQUEST;
using ola::slp::SLPPacketBuilder;
using ola::slp::SLP_OK;
using ola::slp::SLP_REQUEST_MCAST;
using ola::slp::ScopeSet;
using ola::slp::ServiceReplyPacket;
using ola::slp::xid_t;
using std::auto_ptr;

const char RDMNET_DEVICE_SERVICE[] = "service:rdmnet-device";
const ScopeSet RDMNET_SCOPES("rdmnet");

/**
 * Build a packet containing length number of 'data' elements.
 */
void BuildNLengthPacket(BigEndianOutputStream *output, uint8_t data,
                        unsigned int length) {
  for (unsigned int i = 0; i < length; i++)
    (*output) << data;
}

/**
 * Write an SLPString to an BigEndianInputStream. This allows the size of the
 * string to be larger than the contents, in an attempt to trigger an overflow
 * on the remote end.
 */
void WriteOverflowString(BigEndianOutputStream *output,
                         unsigned int header_size,
                         unsigned int actual_size) {
  *output << static_cast<uint16_t>(header_size);
  string data(actual_size, 'a');
  output->Write(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}


/**
 * Verify an SrvRply contains no URLs
 */
TestCase::TestState VerifyEmptySrvReply(const uint8_t *data,
                                        unsigned int length) {
  ola::io::MemoryBuffer buffer(data, length);
  ola::io::BigEndianInputStream stream(&buffer);

  auto_ptr<const ServiceReplyPacket> reply(
    ola::slp::SLPPacketParser::UnpackServiceReply(&stream));
  if (!reply.get())
    return TestCase::FAILED;

  if (reply->error_code != SLP_OK) {
    OLA_INFO << "Error code is " << static_cast<int>(reply->error_code);
    return TestCase::FAILED;
  }

  if (!reply->url_entries.empty()) {
    OLA_INFO << "Expected no URL entries, received "
             << reply->url_entries.size();
    return TestCase::FAILED;
  }
  return TestCase::PASSED;
}

/**
 * Verify a SLP SrvRply.
 */
TestCase::TestState VerifySrvRply(const IPV4Address &destination_ip,
                                  const uint8_t *data, unsigned int length) {
  ola::io::MemoryBuffer buffer(data, length);
  ola::io::BigEndianInputStream stream(&buffer);

  auto_ptr<const ServiceReplyPacket> reply(
    ola::slp::SLPPacketParser::UnpackServiceReply(&stream));
  if (!reply.get())
    return TestCase::FAILED;

  if (reply->error_code != SLP_OK) {
    OLA_INFO << "Error code is " << static_cast<int>(reply->error_code);
    return TestCase::FAILED;
  }

  if (reply->url_entries.size() != 1) {
    OLA_INFO << "Expected 1 URL entry, received "
             << reply->url_entries.size();
    return TestCase::FAILED;
  }

  const ola::slp::URLEntry &url = reply->url_entries[0];
  OLA_INFO << "Received SrvRply containing " << url;
  const string service = ola::slp::SLPServiceFromURL(url.url());
  if (service != RDMNET_DEVICE_SERVICE) {
    OLA_INFO << "Mismatched SLP service, expected '" << RDMNET_DEVICE_SERVICE
             << "', got '" << service << "'";
    return TestCase::FAILED;
  }

  IPV4Address remote_ip;
  UID uid(0, 0);
  if (!ParseE133URL(url.url(), &uid, &remote_ip)) {
    OLA_INFO << "Failed to extract IP & UID from " << url.url();
    return TestCase::FAILED;
  }

  if (remote_ip != destination_ip) {
    OLA_INFO << "IP in url (" << remote_ip
             << ") does not match that of the target";
    return TestCase::FAILED;
  }
  return TestCase::PASSED;
}


void BuildPRListOverflowSrvRqst(BigEndianOutputStream* output,
                                bool multicast, xid_t xid) {
  // The minimum length (excluding header) for a rdmnet:rdmnet-device request is
  // 29. Let's make this longer so we bypass any simple checks on the remote end
  unsigned int pr_list_size = 40;
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, pr_list_size + 2,
                                   multicast ? SLP_REQUEST_MCAST : 0, xid,
                                   EN_LANGUAGE_TAG);
  WriteOverflowString(output, pr_list_size + 100, pr_list_size);
}


void BuildServiceTypeOverflowSrvRqst(BigEndianOutputStream* output,
                                     bool multicast, xid_t xid) {
  unsigned int service_type_size = 40;
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST,
                                   service_type_size + 4,
                                   multicast ? SLP_REQUEST_MCAST : 0, xid,
                                   EN_LANGUAGE_TAG);
  SLPPacketBuilder::WriteString(output, "");  // pr-list is empty
  WriteOverflowString(output, service_type_size + 100, service_type_size);
}


void BuildScopeListOverflowSrvRqst(BigEndianOutputStream* output,
                                   bool multicast, xid_t xid) {
  unsigned int scope_list_size = 40;
  unsigned int body_size = (
      6 + scope_list_size + sizeof(RDMNET_DEVICE_SERVICE) - 1);
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, body_size,
                                   multicast ? SLP_REQUEST_MCAST : 0, xid,
                                   EN_LANGUAGE_TAG);
  SLPPacketBuilder::WriteString(output, "");  // pr-list is empty
  SLPPacketBuilder::WriteString(output, RDMNET_DEVICE_SERVICE);
  WriteOverflowString(output, scope_list_size + 10, scope_list_size);
}


void BuildPredicateOverflowSrvRqst(BigEndianOutputStream* output,
                                   bool multicast, xid_t xid) {
  unsigned int predicate_size = 40;
  unsigned int scope_size = RDMNET_SCOPES.ToString().size();
  unsigned int body_size = (
      8 + sizeof(RDMNET_DEVICE_SERVICE) - 1 + scope_size + predicate_size);
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, body_size,
                                   multicast ? SLP_REQUEST_MCAST : 0, xid,
                                   EN_LANGUAGE_TAG);
  SLPPacketBuilder::WriteString(output, "");  // pr-list is empty
  SLPPacketBuilder::WriteString(output, RDMNET_DEVICE_SERVICE);
  SLPPacketBuilder::WriteString(output, RDMNET_SCOPES.ToString());
  WriteOverflowString(output, predicate_size + 10, predicate_size);
}

void BuildSPIOverflowSrvRqst(BigEndianOutputStream* output,
                             bool multicast, xid_t xid) {
  const unsigned int service_type_size = sizeof(RDMNET_DEVICE_SERVICE) - 1;
  const string scope = RDMNET_SCOPES.ToString();
  const unsigned int scope_size = scope.size();
  const unsigned int spi_size = 40;
  const unsigned int body_size = 10 + service_type_size + scope_size + spi_size;
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, body_size,
                                   multicast ? SLP_REQUEST_MCAST : 0, xid,
                                   EN_LANGUAGE_TAG);
  SLPPacketBuilder::WriteString(output, "");  // pr-list is empty
  SLPPacketBuilder::WriteString(output, RDMNET_DEVICE_SERVICE);
  SLPPacketBuilder::WriteString(output, scope);
  SLPPacketBuilder::WriteString(output, "");  // predicate is empty
  WriteOverflowString(output, spi_size + 10, spi_size);
}
