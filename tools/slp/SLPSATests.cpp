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
 * SLPSATests.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/io/BigEndianStream.h>
#include <ola/io/MemoryBuffer.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/UID.h>
#include <ola/stl/STLUtils.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "tools/e133/SlpUrlParser.h"
#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/SLPSATestRunner.h"
#include "tools/slp/SLPStrings.h"
#include "tools/slp/ScopeSet.h"
#include "tools/slp/URLEntry.h"
#include "tools/slp/XIDAllocator.h"

using ola::io::BigEndianOutputStream;
using ola::network::IPV4Address;
using ola::rdm::UID;
using ola::slp::EN_LANGUAGE_TAG;
using ola::slp::LANGUAGE_NOT_SUPPORTED;
using ola::slp::PARSE_ERROR;
using ola::slp::SCOPE_NOT_SUPPORTED;
using ola::slp::SERVICE_REPLY;
using ola::slp::SERVICE_REQUEST;
using ola::slp::SLPPacketBuilder;
using ola::slp::SLP_OK;
using ola::slp::SLP_REQUEST_MCAST;
using ola::slp::ScopeSet;
using ola::slp::ServiceReplyPacket;
using ola::slp::xid_t;
using std::auto_ptr;
using std::vector;

static const char RDMNET_DEVICE_SERVICE[] = "service:rdmnet-device";
static const ScopeSet RDMNET_SCOPES("rdmnet");

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
  if (!ParseSlpUrl(url.url(), &uid, &remote_ip)) {
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

/**
 * Try a 0-length UDP packet.
 */
class EmptyPacketTest: public TestCase {
void BuildPacket(BigEndianOutputStream *) {
  SetDestination(UNICAST);
  ExpectTimeout();
}
};
REGISTER_TEST(EmptyPacketTest)

/**
 * Try a UDP packet of length 1.
 */
class SingleByteTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();
  BuildNLengthPacket(output, 0, 1);
}
};
REGISTER_TEST(SingleByteTest)

/**
 * A SrvRqst for the service rdmnet-device in scope 'rdmnet'
 */
class SrvRqstTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectResponse(SERVICE_REPLY);

  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), true, pr_list,
                                        RDMNET_DEVICE_SERVICE, RDMNET_SCOPES);
}

TestState VerifyReply(const uint8_t *data, unsigned int length) {
  return VerifySrvRply(GetDestinationIP(), data, length);
}
};
REGISTER_TEST(SrvRqstTest)


/**
 * A SrvRqst to check scope case insensitivity.
 */
class CaseSensitiveScopeSrvRqstTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectResponse(SERVICE_REPLY);
  const ScopeSet test_scopes("RdMnEt");

  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), true, pr_list,
                                        RDMNET_DEVICE_SERVICE, test_scopes);
}

TestState VerifyReply(const uint8_t *data, unsigned int length) {
  return VerifySrvRply(GetDestinationIP(), data, length);
}
};
REGISTER_TEST(CaseSensitiveScopeSrvRqstTest)

/**
 * A SrvRqst to check service-type case insensitivity.
 */
class CaseSensitiveServiceTypeSrvRqstTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectResponse(SERVICE_REPLY);

  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), true, pr_list,
                                        "SerViCe:RdmnEt-dEvicE", RDMNET_SCOPES);
}

TestState VerifyReply(const uint8_t *data, unsigned int length) {
  return VerifySrvRply(GetDestinationIP(), data, length);
}
};
REGISTER_TEST(CaseSensitiveServiceTypeSrvRqstTest)


/**
 * Empty unicast SrvRqst (just the header)
 */
class EmptyUnicastSrvRqstTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, 0, 0, GetXID());
}
};
REGISTER_TEST(EmptyUnicastSrvRqstTest)

/**
 * Empty mulitcast SrvRqst (just the header)
 */
class EmptyMulticastSrvRqstTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, 0,
                                   SLP_REQUEST_MCAST, GetXID());
}
};
REGISTER_TEST(EmptyMulticastSrvRqstTest)


/**
 * A Unicast SrvRqst with a length longer than the packet
 */
class OverflowUnicastSrvRqstTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, 30, 0, GetXID());
}
};
REGISTER_TEST(OverflowUnicastSrvRqstTest)

/**
 * A Multicast SrvRqst with a length longer than the packet
 */
class OverflowMulticastSrvRqstTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, 30,
                                   SLP_REQUEST_MCAST, GetXID());
}
};
REGISTER_TEST(OverflowMulticastSrvRqstTest)

/**
 * A Unicast SrvRqst with a pr-list that overflows.
 */
class UnicastPRListOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);
  // The minimum length (excluding header) for a rdmnet:rdmnet-device request is
  // 29. Let's make this longer so we bypass any simple checks on the remote end
  unsigned int pr_list_size = 40;
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, pr_list_size + 2,
                                   0, GetXID());
  WriteOverflowString(output, pr_list_size + 100, pr_list_size);
}
};
REGISTER_TEST(UnicastPRListOverflow)

/**
 * A Unicast SrvRqst with a service-type that overflows.
 */
class UnicastServiceTypeOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);
  unsigned int service_type_size = 40;
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST,
                                   service_type_size + 4,
                                   0, GetXID());
  SLPPacketBuilder::WriteString(output, "");  // pr-list is empty
  WriteOverflowString(output, service_type_size + 100, service_type_size);
}
};
REGISTER_TEST(UnicastServiceTypeOverflow)

/**
 * A Unicast SrvRqst with a scope-list that overflows.
 */
class UnicastScopeListOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);
  unsigned int scope_list_size = 40;
  unsigned int body_size = (
      6 + scope_list_size + sizeof(RDMNET_DEVICE_SERVICE) - 1);
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, body_size, 0,
                                   GetXID());
  SLPPacketBuilder::WriteString(output, "");  // pr-list is empty
  SLPPacketBuilder::WriteString(output, RDMNET_DEVICE_SERVICE);
  WriteOverflowString(output, scope_list_size + 10, scope_list_size);
}
};
REGISTER_TEST(UnicastScopeListOverflow)

/**
 * A Unicast SrvRqst with a predicate that overflows.
 */
class UnicastPredicateOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);
  unsigned int predicate_size = 40;
  unsigned int scope_size = RDMNET_SCOPES.ToString().size();
  unsigned int body_size = (
      8 + sizeof(RDMNET_DEVICE_SERVICE) - 1 + scope_size + predicate_size);
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, body_size, 0,
                                   GetXID());
  SLPPacketBuilder::WriteString(output, "");  // pr-list is empty
  SLPPacketBuilder::WriteString(output, RDMNET_DEVICE_SERVICE);
  SLPPacketBuilder::WriteString(output, RDMNET_SCOPES.ToString());
  WriteOverflowString(output, predicate_size + 10, predicate_size);
}
};
REGISTER_TEST(UnicastPredicateOverflow)

/**
 * A Unicast SrvRqst with a SPI that overflows.
 */
class UnicastSPIOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);

  const unsigned int service_type_size = sizeof(RDMNET_DEVICE_SERVICE) - 1;
  const string scope = RDMNET_SCOPES.ToString();
  const unsigned int scope_size = scope.size();
  const unsigned int spi_size = 40;
  const unsigned int body_size = 10 + service_type_size + scope_size + spi_size;
  SLPPacketBuilder::BuildSLPHeader(output, SERVICE_REQUEST, body_size, 0,
                                   GetXID());
  SLPPacketBuilder::WriteString(output, "");  // pr-list is empty
  SLPPacketBuilder::WriteString(output, RDMNET_DEVICE_SERVICE);
  SLPPacketBuilder::WriteString(output, scope);
  SLPPacketBuilder::WriteString(output, "");  // predicate is empty
  WriteOverflowString(output, spi_size + 10, spi_size);
}
};
REGISTER_TEST(UnicastSPIOverflow)

/**
 * Try a multicast request with the target's IP in the PR List
 */
class SrvRqstPRListTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();

  pr_list.insert(GetDestinationIP());
  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), true, pr_list,
                                        RDMNET_DEVICE_SERVICE, RDMNET_SCOPES);
}
};
REGISTER_TEST(SrvRqstPRListTest)


/**
 * Try a multicast request with the target's IP in the PR List.
 * The PR list also contains non-ipv4 addresses.
 */
class SrvRqstInvalidPRListTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();

  string pr_list_str = "foo," + GetDestinationIP().ToString() + ",bar";
  pr_list.insert(GetDestinationIP());
  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), true, pr_list_str,
                                        RDMNET_DEVICE_SERVICE, RDMNET_SCOPES,
                                        EN_LANGUAGE_TAG, "");
}
};
REGISTER_TEST(SrvRqstInvalidPRListTest)


/**
 * Try a unicast SrvRqst with a different scope.
 */
class DefaultScopeUnicastTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, SCOPE_NOT_SUPPORTED);

  ScopeSet default_scope("default");
  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), false, pr_list,
                                        RDMNET_DEVICE_SERVICE, default_scope);
}
};
REGISTER_TEST(DefaultScopeUnicastTest)


/**
 * Try a multicast SrvRqst with a different scope.
 */
class DefaultScopeMulticastTest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();

  ScopeSet default_scope("default");
  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), true, pr_list,
                                        RDMNET_DEVICE_SERVICE, default_scope);
}
};
REGISTER_TEST(DefaultScopeMulticastTest)


/**
 * Try a unicast SrvRqst with no service-type.
 */
class MissingServiceTypeUnicastRequest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);

  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), false, pr_list, "",
                                        RDMNET_SCOPES);
}
};
REGISTER_TEST(MissingServiceTypeUnicastRequest)


/**
 * Try a multicast SrvRqst with no service-type.
 */
class MissingServiceTypeMulticastRequest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();

  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), true, pr_list, "",
                                        RDMNET_SCOPES);
}
};
REGISTER_TEST(MissingServiceTypeMulticastRequest)


/**
 * Try a unicast SrvRqst with a different language. Since the language tag only
 * applies to the predicate, and the predicate is empty in this case, this
 * should return a URL Entry.
 */
class NonEnglishUnicastRequest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectResponse(SERVICE_REPLY);

  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), false, pr_list,
                                        RDMNET_DEVICE_SERVICE, RDMNET_SCOPES,
                                        "fr");
}
TestState VerifyReply(const uint8_t *data, unsigned int length) {
  return VerifySrvRply(GetDestinationIP(), data, length);
}
};
REGISTER_TEST(NonEnglishUnicastRequest)


/**
 * Try a multicast SrvRqst with a different language. Since the language tag
 * only applies to the predicate, and the predicate is empty in this case, this
 * should return a URL Entry.
 */
class NonEnglishMulticastRequest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectResponse(SERVICE_REPLY);

  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), true, pr_list,
                                        RDMNET_DEVICE_SERVICE, RDMNET_SCOPES,
                                        "fr");
}
TestState VerifyReply(const uint8_t *data, unsigned int length) {
  return VerifySrvRply(GetDestinationIP(), data, length);
}
};
REGISTER_TEST(NonEnglishMulticastRequest)


/**
 * Try a unicast SrvRqst with a predicate. Since E1.33 services can't have
 * attributes, this should return an empty list.
 */
class UnicastPredicateRequest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectResponse(SERVICE_REPLY);

  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), false, pr_list,
                                        RDMNET_DEVICE_SERVICE, RDMNET_SCOPES,
                                        EN_LANGUAGE_TAG, "!(foo=*)");
}
TestState VerifyReply(const uint8_t *data, unsigned int length) {
  return VerifyEmptySrvReply(data, length);
}
};
REGISTER_TEST(UnicastPredicateRequest)


/**
 * Try a multicast SrvRqst with a predicate. Since E1.33 services can't have
 * attributes, the SA should not reply.
 */
class MulticastPredicateRequest: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();

  SLPPacketBuilder::BuildServiceRequest(output, GetXID(), true, pr_list,
                                        RDMNET_DEVICE_SERVICE, RDMNET_SCOPES,
                                        EN_LANGUAGE_TAG, "!(foo=*)");
}
};
REGISTER_TEST(MulticastPredicateRequest)
