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

#include <string>
#include <iostream>
#include <cppunit/extensions/HelperMacros.h>

#include "HeaderSet.h"
#include "CID.h"

using namespace ola::e131;

class HeaderSetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(HeaderSetTest);
  CPPUNIT_TEST(testRootHeader);
  CPPUNIT_TEST(testHeaderSet);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testRootHeader();
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

  // test copy and assign
  CPPUNIT_ASSERT(cid == header.GetCid());
  RootHeader header2 = header;
  CPPUNIT_ASSERT(cid == header2.GetCid());
  CPPUNIT_ASSERT(header2 == header);
  RootHeader header3(header);
  CPPUNIT_ASSERT(cid == header3.GetCid());
  CPPUNIT_ASSERT(header3 == header);
}

/*
 * Check that the header set works
 */
void HeaderSetTest::testHeaderSet() {
  HeaderSet headers;
  RootHeader root_header;

  // test the root header component
  CID cid;
  cid.Generate();
  root_header.SetCid(cid);
  headers.SetRootHeader(root_header);
  CPPUNIT_ASSERT(root_header == headers.GetRootHeader());

  // test copy and assign
  HeaderSet headers2 = headers;
  CPPUNIT_ASSERT(root_header == headers2.GetRootHeader());
  CPPUNIT_ASSERT(headers2 == headers);
  HeaderSet headers3(headers);
  CPPUNIT_ASSERT(root_header == headers3.GetRootHeader());
  CPPUNIT_ASSERT(headers3 == headers);
}
