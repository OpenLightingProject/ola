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

#include <ola/Logging.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/NetworkUtils.h>
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
    int FatalStop() { CPPUNIT_ASSERT(false); }

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
  Closure *stop_closure = NewClosure(this, &UDPTransportTest::Stop);
  MockInflator inflator(cid, stop_closure);
  ola::network::Interface interface;
  UDPTransport transport(&inflator);
  CPPUNIT_ASSERT(transport.Init(interface));
  m_ss->AddSocket(transport.GetSocket());

  PDUBlock<PDU> pdu_block;
  MockPDU mock_pdu(4, 8);
  pdu_block.AddPDU(&mock_pdu);

  struct sockaddr_in destination;
  destination.sin_family = AF_INET;
  destination.sin_port = htons(UDPTransport::ACN_PORT);
  ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
  ola::network::StringToAddress("255.255.255.255", destination.sin_addr);
  CPPUNIT_ASSERT(transport.Send(pdu_block, destination));

  SingleUseClosure *closure =
    NewSingleClosure(this, &UDPTransportTest::FatalStop);
  m_ss->RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS, closure);
  m_ss->Run();
  delete stop_closure;
  delete closure;
}


} // e131
} // ola
