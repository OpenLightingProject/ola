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

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <iostream>

#include "plugins/e131/e131/HeaderSet.h"
#include "plugins/e131/e131/CID.h"

using ola::plugin::e131::CID;
using ola::plugin::e131::DMPHeader;
using ola::plugin::e131::E131Header;
using ola::plugin::e131::FOUR_BYTES;
using ola::plugin::e131::HeaderSet;
using ola::plugin::e131::NON_RANGE;
using ola::plugin::e131::ONE_BYTES;
using ola::plugin::e131::RANGE_EQUAL;
using ola::plugin::e131::RootHeader;

class HeaderSetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(HeaderSetTest);
  CPPUNIT_TEST(testRootHeader);
  CPPUNIT_TEST(testE131Header);
  CPPUNIT_TEST(testDMPHeader);
  CPPUNIT_TEST(testHeaderSet);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testRootHeader();
    void testE131Header();
    void testDMPHeader();
    void testHeaderSet();
};


CPPUNIT_TEST_SUITE_REGISTRATION(HeaderSetTest);

/*
 * Check that the root header works.
 */
void HeaderSetTest::testRootHeader() {
  CID cid;
  cid.Generate();
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

  // test copy and assign
  E131Header header2 = header;
  CPPUNIT_ASSERT(header.Source() == header2.Source());
  CPPUNIT_ASSERT_EQUAL(header.Priority(), header2.Priority());
  CPPUNIT_ASSERT_EQUAL(header.Sequence(), header2.Sequence());
  CPPUNIT_ASSERT_EQUAL(header.Universe(), header2.Universe());

  E131Header header3(header);
  CPPUNIT_ASSERT(header.Source() == header3.Source());
  CPPUNIT_ASSERT_EQUAL(header.Priority(), header3.Priority());
  CPPUNIT_ASSERT_EQUAL(header.Sequence(), header3.Sequence());
  CPPUNIT_ASSERT_EQUAL(header.Universe(), header3.Universe());
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
  DMPHeader dmp_header(false, false, NON_RANGE, ONE_BYTES);

  // test the root header component
  CID cid;
  cid.Generate();
  root_header.SetCid(cid);
  headers.SetRootHeader(root_header);
  CPPUNIT_ASSERT(root_header == headers.GetRootHeader());

  // test the E1.31 header component
  headers.SetE131Header(e131_header);
  CPPUNIT_ASSERT(e131_header == headers.GetE131Header());

  // test the DMP headers component
  headers.SetDMPHeader(dmp_header);
  CPPUNIT_ASSERT(dmp_header == headers.GetDMPHeader());

  // test assign
  HeaderSet headers2 = headers;
  CPPUNIT_ASSERT(root_header == headers2.GetRootHeader());
  CPPUNIT_ASSERT(e131_header == headers2.GetE131Header());
  CPPUNIT_ASSERT(dmp_header == headers2.GetDMPHeader());
  CPPUNIT_ASSERT(headers2 == headers);

  // test copy
  HeaderSet headers3(headers);
  CPPUNIT_ASSERT(root_header == headers3.GetRootHeader());
  CPPUNIT_ASSERT(e131_header == headers3.GetE131Header());
  CPPUNIT_ASSERT(dmp_header == headers3.GetDMPHeader());
  CPPUNIT_ASSERT(headers3 == headers);
}
