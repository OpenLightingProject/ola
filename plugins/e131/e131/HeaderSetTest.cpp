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
  CPPUNIT_ASSERT(IPV4Address::FromString("192.168.1.1", &address));
  TransportHeader header(address, port);
  CPPUNIT_ASSERT(address == header.SourceIP());
  CPPUNIT_ASSERT_EQUAL(port, header.SourcePort());

  // test copy and assign
  TransportHeader header2 = header;
  CPPUNIT_ASSERT(address == header2.SourceIP());
  CPPUNIT_ASSERT_EQUAL(port, header2.SourcePort());
  CPPUNIT_ASSERT(header2 == header);
  TransportHeader header3(header);
  CPPUNIT_ASSERT(address == header3.SourceIP());
  CPPUNIT_ASSERT_EQUAL(port, header3.SourcePort());
  CPPUNIT_ASSERT(header3 == header);
}


/*
 * Check that the root header works.
 */
void HeaderSetTest::testRootHeader() {
  CID cid = CID::Generate();
  RootHeader header;
  header.SetCid(cid);
  CPPUNIT_ASSERT(cid == header.GetCid());

  // test copy and assign
  RootHeader header2 = header;
  CPPUNIT_ASSERT(cid == header2.GetCid());
  CPPUNIT_ASSERT(header2 == header);
  RootHeader header3(header);
  CPPUNIT_ASSERT(cid == header3.GetCid());
  CPPUNIT_ASSERT(header3 == header);
}


/*
 * test the E1.31 Header
 */
void HeaderSetTest::testE131Header() {
  E131Header header("foo", 1, 2, 2050);
  CPPUNIT_ASSERT("foo" == header.Source());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, header.Priority());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 2, header.Sequence());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 2050, header.Universe());
  CPPUNIT_ASSERT_EQUAL(false, header.PreviewData());
  CPPUNIT_ASSERT_EQUAL(false, header.StreamTerminated());
  CPPUNIT_ASSERT(!header.UsingRev2());

  // test copy and assign
  E131Header header2 = header;
  CPPUNIT_ASSERT(header.Source() == header2.Source());
  CPPUNIT_ASSERT_EQUAL(header.Priority(), header2.Priority());
  CPPUNIT_ASSERT_EQUAL(header.Sequence(), header2.Sequence());
  CPPUNIT_ASSERT_EQUAL(header.Universe(), header2.Universe());
  CPPUNIT_ASSERT_EQUAL(false, header2.PreviewData());
  CPPUNIT_ASSERT_EQUAL(false, header2.StreamTerminated());
  CPPUNIT_ASSERT(!header2.UsingRev2());

  E131Header header3(header);
  CPPUNIT_ASSERT(header.Source() == header3.Source());
  CPPUNIT_ASSERT_EQUAL(header.Priority(), header3.Priority());
  CPPUNIT_ASSERT_EQUAL(header.Sequence(), header3.Sequence());
  CPPUNIT_ASSERT_EQUAL(header.Universe(), header3.Universe());
  CPPUNIT_ASSERT(header == header3);

  // test a rev 2 header
  E131Rev2Header header_rev2("foo", 1, 2, 2050);
  CPPUNIT_ASSERT("foo" == header_rev2.Source());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, header_rev2.Priority());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 2, header_rev2.Sequence());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 2050, header_rev2.Universe());
  CPPUNIT_ASSERT(header_rev2.UsingRev2());
  CPPUNIT_ASSERT(!(header == header_rev2));

  E131Rev2Header header2_rev2 = header_rev2;
  CPPUNIT_ASSERT(header2_rev2 == header_rev2);

  // test a header with the special bits set
  E131Header header4("foo", 1, 2, 2050, true, true);
  CPPUNIT_ASSERT("foo" == header4.Source());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, header4.Priority());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 2, header4.Sequence());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 2050, header4.Universe());
  CPPUNIT_ASSERT_EQUAL(true, header4.PreviewData());
  CPPUNIT_ASSERT_EQUAL(true, header4.StreamTerminated());
  CPPUNIT_ASSERT(!header4.UsingRev2());
}


/*
 * test the E1.33 Header
 */
void HeaderSetTest::testE133Header() {
  E133Header header("foo", 1, 2, 2050);
  CPPUNIT_ASSERT("foo" == header.Source());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, header.Priority());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 2, header.Sequence());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 2050, header.Universe());
  CPPUNIT_ASSERT_EQUAL(false, header.IsManagement());
  CPPUNIT_ASSERT_EQUAL(false, header.IsSquawk());

  // test copy and assign
  E133Header header2 = header;
  CPPUNIT_ASSERT(header.Source() == header2.Source());
  CPPUNIT_ASSERT_EQUAL(header.Priority(), header2.Priority());
  CPPUNIT_ASSERT_EQUAL(header.Sequence(), header2.Sequence());
  CPPUNIT_ASSERT_EQUAL(header.Universe(), header2.Universe());
  CPPUNIT_ASSERT_EQUAL(false, header2.IsManagement());
  CPPUNIT_ASSERT_EQUAL(false, header2.IsSquawk());

  E133Header header3(header);
  CPPUNIT_ASSERT(header.Source() == header3.Source());
  CPPUNIT_ASSERT_EQUAL(header.Priority(), header3.Priority());
  CPPUNIT_ASSERT_EQUAL(header.Sequence(), header3.Sequence());
  CPPUNIT_ASSERT_EQUAL(header.Universe(), header3.Universe());
  CPPUNIT_ASSERT(header == header3);

  // test a header with the management bit set
  E133Header header4("foo", 1, 2, 2050, true);
  CPPUNIT_ASSERT("foo" == header4.Source());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, header4.Priority());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 2, header4.Sequence());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 2050, header4.Universe());
  CPPUNIT_ASSERT_EQUAL(true, header4.IsManagement());
  CPPUNIT_ASSERT_EQUAL(false, header4.IsSquawk());

  // test a header with the squawk bit set
  E133Header header5("foo", 1, 2, 2050, false, true);
  CPPUNIT_ASSERT("foo" == header5.Source());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, header5.Priority());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 2, header5.Sequence());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 2050, header5.Universe());
  CPPUNIT_ASSERT_EQUAL(false, header5.IsManagement());
  CPPUNIT_ASSERT_EQUAL(true, header5.IsSquawk());
}


/*
 * test the DMP Header
 */
void HeaderSetTest::testDMPHeader() {
  DMPHeader header(false, false, NON_RANGE, ONE_BYTES);
  CPPUNIT_ASSERT_EQUAL(false, header.IsVirtual());
  CPPUNIT_ASSERT_EQUAL(false, header.IsRelative());
  CPPUNIT_ASSERT_EQUAL(NON_RANGE, header.Type());
  CPPUNIT_ASSERT_EQUAL(ONE_BYTES, header.Size());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, header.Header());
  DMPHeader test_header(0);
  CPPUNIT_ASSERT(test_header == header);

  DMPHeader header2(false, true, RANGE_EQUAL, FOUR_BYTES);
  CPPUNIT_ASSERT_EQUAL(false, header2.IsVirtual());
  CPPUNIT_ASSERT_EQUAL(true, header2.IsRelative());
  CPPUNIT_ASSERT_EQUAL(RANGE_EQUAL, header2.Type());
  CPPUNIT_ASSERT_EQUAL(FOUR_BYTES, header2.Size());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0x62, header2.Header());
  DMPHeader test_header2(0x62);
  CPPUNIT_ASSERT(test_header2 == header2);

  // test copy and assign
  DMPHeader header3 = header;
  CPPUNIT_ASSERT(header3 == header);
  CPPUNIT_ASSERT(header3 != header2);

  DMPHeader header4(header);
  CPPUNIT_ASSERT(header4 == header);
  CPPUNIT_ASSERT(header4 != header2);
}


/*
 * Check that the header set works
 */
void HeaderSetTest::testHeaderSet() {
  HeaderSet headers;
  RootHeader root_header;
  E131Header e131_header("e131", 1, 2, 6001);
  E133Header e133_header("foo", 1, 2, 2050, true);
  DMPHeader dmp_header(false, false, NON_RANGE, ONE_BYTES);

  // test the root header component
  CID cid = CID::Generate();
  root_header.SetCid(cid);
  headers.SetRootHeader(root_header);
  CPPUNIT_ASSERT(root_header == headers.GetRootHeader());

  // test the E1.31 header component
  headers.SetE131Header(e131_header);
  CPPUNIT_ASSERT(e131_header == headers.GetE131Header());

  // test the E1.33 header component
  headers.SetE133Header(e133_header);
  CPPUNIT_ASSERT(e133_header == headers.GetE133Header());

  // test the DMP headers component
  headers.SetDMPHeader(dmp_header);
  CPPUNIT_ASSERT(dmp_header == headers.GetDMPHeader());

  // test assign
  HeaderSet headers2 = headers;
  CPPUNIT_ASSERT(root_header == headers2.GetRootHeader());
  CPPUNIT_ASSERT(e131_header == headers2.GetE131Header());
  CPPUNIT_ASSERT(e133_header == headers2.GetE133Header());
  CPPUNIT_ASSERT(dmp_header == headers2.GetDMPHeader());
  CPPUNIT_ASSERT(headers2 == headers);

  // test copy
  HeaderSet headers3(headers);
  CPPUNIT_ASSERT(root_header == headers3.GetRootHeader());
  CPPUNIT_ASSERT(e131_header == headers3.GetE131Header());
  CPPUNIT_ASSERT(e133_header == headers3.GetE133Header());
  CPPUNIT_ASSERT(dmp_header == headers3.GetDMPHeader());
  CPPUNIT_ASSERT(headers3 == headers);
}
