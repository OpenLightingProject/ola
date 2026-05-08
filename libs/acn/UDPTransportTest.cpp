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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * UDPTransportTest.cpp
 * Test fixture for the UDPTransport class
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/Socket.h"
#include "ola/testing/TestUtils.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/UDPTransport.h"

namespace ola {
namespace acn {

using ola::acn::CID;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
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
  std::unique_ptr<Callback0<void> > stop_closure(
      NewCallback(this, &UDPTransportTest::Stop));
  MockInflator inflator(cid, stop_closure.get());

  // setup the socket
  ola::network::UDPSocket socket;
  OLA_ASSERT(socket.Init());
  OLA_ASSERT(socket.Bind(IPV4SocketAddress(IPV4Address::Loopback(), 0)));
  OLA_ASSERT(socket.EnableBroadcast());

  // Get the port we bound to.
  IPV4SocketAddress local_address;
  OLA_ASSERT(socket.GetSocketAddress(&local_address));

  IncomingUDPTransport incoming_udp_transport(&socket, &inflator);
  socket.SetOnData(NewCallback(&incoming_udp_transport,
                               &IncomingUDPTransport::Receive));
  OLA_ASSERT(m_ss->AddReadDescriptor(&socket));

  // outgoing transport
  OutgoingUDPTransportImpl udp_transport_impl(&socket);
  OutgoingUDPTransport outgoing_udp_transport(&udp_transport_impl,
      IPV4Address::Loopback(), local_address.Port());

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
}  // namespace acn
}  // namespace ola
