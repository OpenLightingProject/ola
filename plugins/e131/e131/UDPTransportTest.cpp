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
 * UDPTransportTest.cpp
 * Test fixture for the UDPTransport class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <arpa/inet.h>
#include <ola/Logging.h>
#include <ola/network/SelectServer.h>
#include <ola/network/Socket.h>
#include <cppunit/extensions/HelperMacros.h>

#include "UDPTransport.h"
#include "PDUTestCommon.h"

namespace ola {
namespace e131 {

class UDPTransportTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UDPTransportTest);
  CPPUNIT_TEST(testUDPTransport);
  CPPUNIT_TEST_SUITE_END();

  public:
    UDPTransportTest(): TestFixture(), m_ss(NULL) {}
    void testUDPTransport();
    void setUp();
    void tearDown();
    int Stop();
  private:
    ola::network::SelectServer *m_ss;
    static const int ABORT_TIMEOUT_IN_MS = 1000;
};

CPPUNIT_TEST_SUITE_REGISTRATION(UDPTransportTest);

void UDPTransportTest::setUp() {
  m_ss = new ola::network::SelectServer();
}

void UDPTransportTest::tearDown() {
  delete m_ss;
}

int UDPTransportTest::Stop() {
  if (m_ss)
    m_ss->Terminate();
}


/*
 * Test the UDPTransport
 */
void UDPTransportTest::testUDPTransport() {
  CID cid;
  MockInflator inflator(cid, NewClosure(this, &UDPTransportTest::Stop));
  UDPTransport transport(&inflator);
  CPPUNIT_ASSERT(transport.Init());
  m_ss->AddSocket(transport.GetSocket());

  PDUBlock<PDU> pdu_block;
  MockPDU mock_pdu(4, 8);
  pdu_block.AddPDU(&mock_pdu);

  struct sockaddr_in destination;
  destination.sin_family = AF_INET;
  destination.sin_port = htons(UDPTransport::ACN_PORT);
  inet_aton("255.255.255.255", &destination.sin_addr);
  CPPUNIT_ASSERT(transport.Send(pdu_block, destination));

  m_ss->RegisterSingleTimeout(
      ABORT_TIMEOUT_IN_MS,
      NewSingleClosure(this, &UDPTransportTest::Stop));
  m_ss->Run();
}


} // e131
} // ola
