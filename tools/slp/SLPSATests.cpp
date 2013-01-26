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

#include "tools/e133/SlpUrlParser.h"
#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/SLPSATestHelpers.h"
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
  BuildPRListOverflowSrvRqst(output, false, GetXID());
}
};
REGISTER_TEST(UnicastPRListOverflow)


/**
 * A Multicast SrvRqst with a pr-list that overflows.
 */
class MulticastPRListOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();
  BuildPRListOverflowSrvRqst(output, true, GetXID());
}
};
REGISTER_TEST(MulticastPRListOverflow)

/**
 * A Unicast SrvRqst with a service-type that overflows.
 */
class UnicastServiceTypeOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);
  BuildServiceTypeOverflowSrvRqst(output, false, GetXID());
}
};
REGISTER_TEST(UnicastServiceTypeOverflow)


/**
 * A Multicast SrvRqst with a service-type that overflows.
 */
class MulticastServiceTypeOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();
  BuildServiceTypeOverflowSrvRqst(output, true, GetXID());
}
};
REGISTER_TEST(MulticastServiceTypeOverflow)

/**
 * A Unicast SrvRqst with a scope-list that overflows.
 */
class UnicastScopeListOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);
  BuildScopeListOverflowSrvRqst(output, false, GetXID());
}
};
REGISTER_TEST(UnicastScopeListOverflow)

/**
 * A Multicast SrvRqst with a scope-list that overflows.
 */
class MulticastScopeListOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();
  BuildScopeListOverflowSrvRqst(output, true, GetXID());
}
};
REGISTER_TEST(MulticastScopeListOverflow)

/**
 * A Unicast SrvRqst with a predicate that overflows.
 */
class UnicastPredicateOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);
  BuildPredicateOverflowSrvRqst(output, false, GetXID());
}
};
REGISTER_TEST(UnicastPredicateOverflow)

/**
 * A Multicast SrvRqst with a predicate that overflows.
 */
class MulticastPredicateOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();
  BuildPredicateOverflowSrvRqst(output, true, GetXID());
}
};
REGISTER_TEST(MulticastPredicateOverflow)

/**
 * A Unicast SrvRqst with a SPI that overflows.
 */
class UnicastSPIOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(UNICAST);
  ExpectError(SERVICE_REPLY, PARSE_ERROR);
  BuildSPIOverflowSrvRqst(output, false, GetXID());
}
};
REGISTER_TEST(UnicastSPIOverflow)

/**
 * A Multicast SrvRqst with a SPI that overflows.
 */
class MulticastSPIOverflow: public TestCase {
void BuildPacket(BigEndianOutputStream *output) {
  SetDestination(MULTICAST);
  ExpectTimeout();
  BuildSPIOverflowSrvRqst(output, true, GetXID());
}
};
REGISTER_TEST(MulticastSPIOverflow)

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
