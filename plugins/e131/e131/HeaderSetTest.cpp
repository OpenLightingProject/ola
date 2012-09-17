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
 * HeaderSetTest.cpp
 * Test fixture for the HeaderSet class
 * Copyright (C) 2007-2009 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <iostream>

#include "ola/testing/TestUtils.h"

#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/HeaderSet.h"

using ola::network::IPV4Address;
using ola::plugin::e131::CID;
using ola::plugin::e131::DMPHeader;
using ola::plugin::e131::E131Header;
using ola::plugin::e131::E131Rev2Header;
using ola::plugin::e131::E133Header;
using ola::plugin::e131::FOUR_BYTES;
using ola::plugin::e131::HeaderSet;
using ola::plugin::e131::NON_RANGE;
using ola::plugin::e131::ONE_BYTES;
using ola::plugin::e131::RANGE_EQUAL;
using ola::plugin::e131::RootHeader;
using ola::plugin::e131::TransportHeader;

class HeaderSetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(HeaderSetTest);
  CPPUNIT_TEST(testTransportHeader);
  CPPUNIT_TEST(testRootHeader);
  CPPUNIT_TEST(testE131Header);
  CPPUNIT_TEST(testE133Header);
  CPPUNIT_TEST(testDMPHeader);
  CPPUNIT_TEST(testHeaderSet);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testRootHeader();
    void testTransportHeader();
    void testE131Header();
    void testE133Header();
    void testDMPHeader();
    void testHeaderSet();
};


CPPUNIT_TEST_SUITE_REGISTRATION(HeaderSetTest);

/*
 * Check that the transport header works.
 */
void HeaderSetTest::testTransportHeader() {
  IPV4Address address;
  uint16_t port = 42;
  OLA_ASSERT(IPV4Address::FromString("192.168.1.1", &address));
  TransportHeader header(address, port, TransportHeader::UDP);
  OLA_ASSERT(address == header.SourceIP());
  OLA_ASSERT_EQ(port, header.SourcePort());
  OLA_ASSERT_EQ(TransportHeader::UDP, header.Transport());

  // test copy and assign
  TransportHeader header2 = header;
  OLA_ASSERT(address == header2.SourceIP());
  OLA_ASSERT_EQ(port, header2.SourcePort());
  OLA_ASSERT(header2 == header);
  TransportHeader header3(header);
  OLA_ASSERT(address == header3.SourceIP());
  OLA_ASSERT_EQ(port, header3.SourcePort());
  OLA_ASSERT(header3 == header);
}


/*
 * Check that the root header works.
 */
void HeaderSetTest::testRootHeader() {
  CID cid = CID::Generate();
  RootHeader header;
  header.SetCid(cid);
  OLA_ASSERT(cid == header.GetCid());

  // test copy and assign
  RootHeader header2 = header;
  OLA_ASSERT(cid == header2.GetCid());
  OLA_ASSERT(header2 == header);
  RootHeader header3(header);
  OLA_ASSERT(cid == header3.GetCid());
  OLA_ASSERT(header3 == header);
}


/*
 * test the E1.31 Header
 */
void HeaderSetTest::testE131Header() {
  E131Header header("foo", 1, 2, 2050);
  OLA_ASSERT("foo" == header.Source());
  OLA_ASSERT_EQ((uint8_t) 1, header.Priority());
  OLA_ASSERT_EQ((uint8_t) 2, header.Sequence());
  OLA_ASSERT_EQ((uint16_t) 2050, header.Universe());
  OLA_ASSERT_EQ(false, header.PreviewData());
  OLA_ASSERT_EQ(false, header.StreamTerminated());
  OLA_ASSERT_FALSE(header.UsingRev2());

  // test copy and assign
  E131Header header2 = header;
  OLA_ASSERT(header.Source() == header2.Source());
  OLA_ASSERT_EQ(header.Priority(), header2.Priority());
  OLA_ASSERT_EQ(header.Sequence(), header2.Sequence());
  OLA_ASSERT_EQ(header.Universe(), header2.Universe());
  OLA_ASSERT_EQ(false, header2.PreviewData());
  OLA_ASSERT_EQ(false, header2.StreamTerminated());
  OLA_ASSERT_FALSE(header2.UsingRev2());

  E131Header header3(header);
  OLA_ASSERT(header.Source() == header3.Source());
  OLA_ASSERT_EQ(header.Priority(), header3.Priority());
  OLA_ASSERT_EQ(header.Sequence(), header3.Sequence());
  OLA_ASSERT_EQ(header.Universe(), header3.Universe());
  OLA_ASSERT(header == header3);

  // test a rev 2 header
  E131Rev2Header header_rev2("foo", 1, 2, 2050);
  OLA_ASSERT("foo" == header_rev2.Source());
  OLA_ASSERT_EQ((uint8_t) 1, header_rev2.Priority());
  OLA_ASSERT_EQ((uint8_t) 2, header_rev2.Sequence());
  OLA_ASSERT_EQ((uint16_t) 2050, header_rev2.Universe());
  OLA_ASSERT(header_rev2.UsingRev2());
  OLA_ASSERT_FALSE((header == header_rev2));

  E131Rev2Header header2_rev2 = header_rev2;
  OLA_ASSERT(header2_rev2 == header_rev2);

  // test a header with the special bits set
  E131Header header4("foo", 1, 2, 2050, true, true);
  OLA_ASSERT("foo" == header4.Source());
  OLA_ASSERT_EQ((uint8_t) 1, header4.Priority());
  OLA_ASSERT_EQ((uint8_t) 2, header4.Sequence());
  OLA_ASSERT_EQ((uint16_t) 2050, header4.Universe());
  OLA_ASSERT_EQ(true, header4.PreviewData());
  OLA_ASSERT_EQ(true, header4.StreamTerminated());
  OLA_ASSERT_FALSE(header4.UsingRev2());
}


/*
 * test the E1.33 Header
 */
void HeaderSetTest::testE133Header() {
  E133Header header("foo", 9840, 2, false);
  OLA_ASSERT("foo" == header.Source());
  OLA_ASSERT_EQ((uint32_t) 9840, header.Sequence());
  OLA_ASSERT_EQ((uint16_t) 2, header.Endpoint());
  OLA_ASSERT_EQ(false, header.RxAcknowledge());

  // test copy and assign
  E133Header header2 = header;
  OLA_ASSERT(header.Source() == header2.Source());
  OLA_ASSERT_EQ(header.Sequence(), header2.Sequence());
  OLA_ASSERT_EQ(header.Endpoint(), header2.Endpoint());
  OLA_ASSERT_EQ(false, header2.RxAcknowledge());

  E133Header header3(header);
  OLA_ASSERT(header.Source() == header3.Source());
  OLA_ASSERT_EQ(header.Sequence(), header3.Sequence());
  OLA_ASSERT_EQ(header.Endpoint(), header3.Endpoint());
  OLA_ASSERT(header == header3);

  // test a header with the RX ack bit set
  E133Header header4("foo", 123456, 42, true);
  OLA_ASSERT("foo" == header4.Source());
  OLA_ASSERT_EQ((uint32_t) 123456, header4.Sequence());
  OLA_ASSERT_EQ((uint16_t) 42, header4.Endpoint());
  OLA_ASSERT_EQ(true, header4.RxAcknowledge());

  // test a header with the squawk bit set
  E133Header header5("foo", 123456, 42, false);
  OLA_ASSERT("foo" == header5.Source());
  OLA_ASSERT_EQ((uint32_t) 123456, header5.Sequence());
  OLA_ASSERT_EQ((uint16_t) 42, header5.Endpoint());
  OLA_ASSERT_EQ(false, header5.RxAcknowledge());
}


/*
 * test the DMP Header
 */
void HeaderSetTest::testDMPHeader() {
  DMPHeader header(false, false, NON_RANGE, ONE_BYTES);
  OLA_ASSERT_EQ(false, header.IsVirtual());
  OLA_ASSERT_EQ(false, header.IsRelative());
  OLA_ASSERT_EQ(NON_RANGE, header.Type());
  OLA_ASSERT_EQ(ONE_BYTES, header.Size());
  OLA_ASSERT_EQ((uint8_t) 0, header.Header());
  DMPHeader test_header(0);
  OLA_ASSERT(test_header == header);

  DMPHeader header2(false, true, RANGE_EQUAL, FOUR_BYTES);
  OLA_ASSERT_EQ(false, header2.IsVirtual());
  OLA_ASSERT_EQ(true, header2.IsRelative());
  OLA_ASSERT_EQ(RANGE_EQUAL, header2.Type());
  OLA_ASSERT_EQ(FOUR_BYTES, header2.Size());
  OLA_ASSERT_EQ((uint8_t) 0x62, header2.Header());
  DMPHeader test_header2(0x62);
  OLA_ASSERT_TRUE(test_header2 == header2);

  // test copy and assign
  DMPHeader header3 = header;
  OLA_ASSERT_TRUE(header3 == header);
  OLA_ASSERT_NE(header3, header2);

  DMPHeader header4(header);
  OLA_ASSERT_TRUE(header4 == header);
  OLA_ASSERT_NE(header4, header2);
}


/*
 * Check that the header set works
 */
void HeaderSetTest::testHeaderSet() {
  HeaderSet headers;
  RootHeader root_header;
  E131Header e131_header("e131", 1, 2, 6001);
  E133Header e133_header("foo", 1, 2050, true);
  DMPHeader dmp_header(false, false, NON_RANGE, ONE_BYTES);

  // test the root header component
  CID cid = CID::Generate();
  root_header.SetCid(cid);
  headers.SetRootHeader(root_header);
  OLA_ASSERT(root_header == headers.GetRootHeader());

  // test the E1.31 header component
  headers.SetE131Header(e131_header);
  OLA_ASSERT(e131_header == headers.GetE131Header());

  // test the E1.33 header component
  headers.SetE133Header(e133_header);
  OLA_ASSERT(e133_header == headers.GetE133Header());

  // test the DMP headers component
  headers.SetDMPHeader(dmp_header);
  OLA_ASSERT(dmp_header == headers.GetDMPHeader());

  // test assign
  HeaderSet headers2 = headers;
  OLA_ASSERT(root_header == headers2.GetRootHeader());
  OLA_ASSERT(e131_header == headers2.GetE131Header());
  OLA_ASSERT(e133_header == headers2.GetE133Header());
  OLA_ASSERT(dmp_header == headers2.GetDMPHeader());
  OLA_ASSERT(headers2 == headers);

  // test copy
  HeaderSet headers3(headers);
  OLA_ASSERT(root_header == headers3.GetRootHeader());
  OLA_ASSERT(e131_header == headers3.GetE131Header());
  OLA_ASSERT(e133_header == headers3.GetE133Header());
  OLA_ASSERT(dmp_header == headers3.GetDMPHeader());
  OLA_ASSERT(headers3 == headers);
}
