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
 * RootSenderTest.cpp
 * Test fixture for the RootSender class
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/io/SelectServer.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/Socket.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/RootInflator.h"
#include "libs/acn/RootSender.h"
#include "libs/acn/UDPTransport.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace acn {

using ola::acn::CID;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;

class RootSenderTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RootSenderTest);
  CPPUNIT_TEST(testRootSender);
  CPPUNIT_TEST(testRootSenderWithCustomCID);
  CPPUNIT_TEST_SUITE_END();

 public:
    RootSenderTest(): TestFixture(), m_ss(NULL) {}
    void testRootSender();
    void testRootSenderWithCustomCID();
    void setUp();
    void tearDown();
    void Stop();
    void FatalStop() { OLA_ASSERT(false); }

 private:
    void testRootSenderWithCIDs(const CID &root_cid, const CID &send_cid);
    ola::io::SelectServer *m_ss;
    static const int ABORT_TIMEOUT_IN_MS = 1000;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RootSenderTest);

void RootSenderTest::setUp() {
  m_ss = new ola::io::SelectServer();
}

void RootSenderTest::tearDown() {
  delete m_ss;
}

void RootSenderTest::Stop() {
  if (m_ss)
    m_ss->Terminate();
}


/*
 * Test the RootSender
 */
void RootSenderTest::testRootSender() {
  CID cid = CID::Generate();
  testRootSenderWithCIDs(cid, cid);
}


/*
 * Test the method to send using a custom cid works
 */
void RootSenderTest::testRootSenderWithCustomCID() {
  CID cid = CID::Generate();
  CID send_cid = CID::Generate();
  testRootSenderWithCIDs(cid, send_cid);
}


void RootSenderTest::testRootSenderWithCIDs(const CID &root_cid,
                                            const CID &send_cid) {
  std::auto_ptr<Callback0<void> > stop_closure(
      NewCallback(this, &RootSenderTest::Stop));

  // inflators
  MockInflator inflator(send_cid, stop_closure.get());
  RootInflator root_inflator;
  OLA_ASSERT(root_inflator.AddInflator(&inflator));

  // sender
  RootSender root_sender(root_cid);

  // setup the socket
  ola::network::UDPSocket socket;
  OLA_ASSERT(socket.Init());
  OLA_ASSERT(
      socket.Bind(IPV4SocketAddress(IPV4Address::Loopback(), 0)));
  OLA_ASSERT(socket.EnableBroadcast());

  IncomingUDPTransport incoming_udp_transport(&socket, &root_inflator);
  socket.SetOnData(NewCallback(&incoming_udp_transport,
                               &IncomingUDPTransport::Receive));
  OLA_ASSERT(m_ss->AddReadDescriptor(&socket));

  // Get the port we bound to.
  IPV4SocketAddress local_address;
  OLA_ASSERT(socket.GetSocketAddress(&local_address));

  OutgoingUDPTransportImpl udp_transport_impl(&socket);
  OutgoingUDPTransport outgoing_udp_transport(&udp_transport_impl,
      IPV4Address::Loopback(), local_address.Port());

  // now actually send some data
  MockPDU mock_pdu(4, 8);

  if (root_cid == send_cid)
    OLA_ASSERT(root_sender.SendPDU(MockPDU::TEST_VECTOR,
                                       mock_pdu,
                                       &outgoing_udp_transport));
  else
    OLA_ASSERT(root_sender.SendPDU(MockPDU::TEST_VECTOR,
                                       mock_pdu,
                                       send_cid,
                                       &outgoing_udp_transport));

  SingleUseCallback0<void> *closure =
    NewSingleCallback(this, &RootSenderTest::FatalStop);
  m_ss->RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS, closure);
  m_ss->Run();
}
}  // namespace acn
}  // namespace ola
