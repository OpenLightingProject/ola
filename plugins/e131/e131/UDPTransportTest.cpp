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

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/testing/TestUtils.h"

#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/Socket.h"
#include "plugins/e131/e131/PDUTestCommon.h"
#include "plugins/e131/e131/UDPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::HostToNetwork;
using ola::network::IPV4SocketAddress;

class UDPTransportTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UDPTransportTest);
  CPPUNIT_TEST(testUDPTransport);
  CPPUNIT_TEST_SUITE_END();

  public:
    UDPTransportTest(): TestFixture(), m_ss(NULL) {}
    void testUDPTransport();
    void setUp();
    void tearDown();
    void Stop();
    void FatalStop() { OLA_ASSERT(false); }

  private:
    ola::io::SelectServer *m_ss;
    static const int ABORT_TIMEOUT_IN_MS = 1000;
};

CPPUNIT_TEST_SUITE_REGISTRATION(UDPTransportTest);

void UDPTransportTest::setUp() {
  m_ss = new ola::io::SelectServer();
}

void UDPTransportTest::tearDown() {
  delete m_ss;
}

void UDPTransportTest::Stop() {
  if (m_ss)
    m_ss->Terminate();
}


/*
 * Test the UDPTransport
 */
void UDPTransportTest::testUDPTransport() {
  CID cid;
  std::auto_ptr<Callback0<void> > stop_closure(
      NewCallback(this, &UDPTransportTest::Stop));
  MockInflator inflator(cid, stop_closure.get());

  // setup the socket
  ola::network::UDPSocket socket;
  OLA_ASSERT(socket.Init());
  OLA_ASSERT(
      socket.Bind(IPV4SocketAddress(IPV4Address::WildCard(), ACN_PORT)));
  OLA_ASSERT(socket.EnableBroadcast());

  IncomingUDPTransport incoming_udp_transport(&socket, &inflator);
  socket.SetOnData(NewCallback(&incoming_udp_transport,
                               &IncomingUDPTransport::Receive));
  OLA_ASSERT(m_ss->AddReadDescriptor(&socket));

  // outgoing transport
  IPV4Address addr;
  OLA_ASSERT(IPV4Address::FromString("255.255.255.255", &addr));

  OutgoingUDPTransportImpl udp_transport_impl(&socket);
  OutgoingUDPTransport outgoing_udp_transport(&udp_transport_impl, addr);

  // now actually send some data
  PDUBlock<PDU> pdu_block;
  MockPDU mock_pdu(4, 8);
  pdu_block.AddPDU(&mock_pdu);
  OLA_ASSERT(outgoing_udp_transport.Send(pdu_block));

  SingleUseCallback0<void> *closure =
    NewSingleCallback(this, &UDPTransportTest::FatalStop);
  m_ss->RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS, closure);
  m_ss->Run();
}
}  // e131
}  // plugin
}  // ola
