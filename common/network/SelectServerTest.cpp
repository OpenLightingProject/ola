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
 * SelectServerTest.cpp
 * Test fixture for the Socket classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include <lla/ExportMap.h>
#include <lla/network/SelectServer.h>
#include <lla/network/Socket.h>

using namespace lla::network;
using lla::ExportMap;
using lla::IntegerVariable;

class SelectServerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SelectServerTest);
  CPPUNIT_TEST(testAddRemoveSocket);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testAddRemoveSocket();

  private:
    ExportMap *m_map;
    SelectServer *m_ss;
};


CPPUNIT_TEST_SUITE_REGISTRATION(SelectServerTest);


void SelectServerTest::setUp() {
  m_map = new ExportMap();
  m_ss = new SelectServer(m_map);
}


void SelectServerTest::tearDown() {
  delete m_ss;
  delete m_map;
}


/*
 * Check AddSocket/RemoveSocket works correctly and that the export map is
 * updated.
 */
void SelectServerTest::testAddRemoveSocket() {
  ConnectedSocket bad_socket;
  IntegerVariable *fd_count = m_map->GetIntegerVar(SelectServer::K_FD_VAR);
  CPPUNIT_ASSERT_EQUAL(fd_count->Get(), 0);
  // adding and removin a non-connected socket should fail
  CPPUNIT_ASSERT(!m_ss->AddSocket(&bad_socket, NULL));
  CPPUNIT_ASSERT(!m_ss->RemoveSocket(&bad_socket));

  LoopbackSocket loopback_socket;
  loopback_socket.Init();
  CPPUNIT_ASSERT_EQUAL(fd_count->Get(), 0);
  CPPUNIT_ASSERT(m_ss->AddSocket(&loopback_socket, NULL));
  // Adding a second time should fail
  CPPUNIT_ASSERT(!m_ss->AddSocket(&loopback_socket, NULL));
  CPPUNIT_ASSERT_EQUAL(fd_count->Get(), 1);

  // Check remove works
  CPPUNIT_ASSERT(m_ss->RemoveSocket(&loopback_socket));
  CPPUNIT_ASSERT_EQUAL(fd_count->Get(), 0);
  // Remove again should fail
  CPPUNIT_ASSERT(!m_ss->RemoveSocket(&loopback_socket));
}
