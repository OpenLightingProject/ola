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
 * RootLayerTest.cpp
 * Test fixture for the RootLayer class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <arpa/inet.h>
#include <ola/network/SelectServer.h>
#include <ola/network/InterfacePicker.h>
#include <cppunit/extensions/HelperMacros.h>

#include "PDUTestCommon.h"
#include "RootInflator.h"
#include "RootLayer.h"
#include "UDPTransport.h"

namespace ola {
namespace e131 {

class RootLayerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RootLayerTest);
  CPPUNIT_TEST(testRootLayer);
  CPPUNIT_TEST_SUITE_END();

  public:
    RootLayerTest(): TestFixture(), m_ss(NULL) {}
    void testRootLayer();
    void setUp();
    void tearDown();
    int Stop();
    int FatalStop() { CPPUNIT_ASSERT(false); }

  private:
    ola::network::SelectServer *m_ss;
    static const int ABORT_TIMEOUT_IN_MS = 1000;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RootLayerTest);

void RootLayerTest::setUp() {
  m_ss = new ola::network::SelectServer();
}

void RootLayerTest::tearDown() {
  delete m_ss;
}

int RootLayerTest::Stop() {
  if (m_ss)
    m_ss->Terminate();
}


/*
 * Test the RootLayer
 */
void RootLayerTest::testRootLayer() {
  CID cid = CID::Generate();
  ola::network::Interface interface;
  UDPTransport transport;
  CPPUNIT_ASSERT(transport.Init(interface));
  CPPUNIT_ASSERT(m_ss->AddSocket(transport.GetSocket()));
  RootLayer layer(&transport, cid);

  Closure *stop_closure = NewClosure(this, &RootLayerTest::Stop);
  MockInflator inflator(cid, stop_closure);
  CPPUNIT_ASSERT(layer.AddInflator(&inflator));

  MockPDU mock_pdu(4, 8);
  struct in_addr addr;
  inet_aton("255.255.255.255", &addr);
  CPPUNIT_ASSERT(layer.SendPDU(addr, MockPDU::TEST_VECTOR, mock_pdu));

  SingleUseClosure *closure =
    NewSingleClosure(this, &RootLayerTest::FatalStop);
  m_ss->RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS, closure);
  m_ss->Run();
  delete closure;
  delete stop_closure;
}


} // e131
} // ola
