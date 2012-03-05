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

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "plugins/e131/e131/PDUTestCommon.h"
#include "plugins/e131/e131/RootInflator.h"
#include "plugins/e131/e131/RootLayer.h"
#include "plugins/e131/e131/UDPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::IPV4Address;

class RootLayerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RootLayerTest);
  CPPUNIT_TEST(testRootLayer);
  CPPUNIT_TEST(testRootLayerWithCustomCID);
  CPPUNIT_TEST_SUITE_END();

  public:
    RootLayerTest(): TestFixture(), m_ss(NULL) {}
    void testRootLayer();
    void testRootLayerWithCustomCID();
    void setUp();
    void tearDown();
    void Stop();
    void FatalStop() { CPPUNIT_ASSERT(false); }

  private:
    void testRootLayerWithCIDs(const CID &root_cid, const CID &send_cid);
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

void RootLayerTest::Stop() {
  if (m_ss)
    m_ss->Terminate();
}


/*
 * Test the RootLayer
 */
void RootLayerTest::testRootLayer() {
  CID cid = CID::Generate();
  testRootLayerWithCIDs(cid, cid);
}


/*
 * Test the method to send using a custom cid works
 */
void RootLayerTest::testRootLayerWithCustomCID() {
  CID cid = CID::Generate();
  CID send_cid = CID::Generate();
  testRootLayerWithCIDs(cid, send_cid);
}


void RootLayerTest::testRootLayerWithCIDs(const CID &root_cid,
                                          const CID &send_cid) {
  std::auto_ptr<Callback0<void> > stop_closure(
      NewCallback(this, &RootLayerTest::Stop));

  // inflators
  MockInflator inflator(send_cid, stop_closure.get());
  RootInflator root_inflator;
  CPPUNIT_ASSERT(root_inflator.AddInflator(&inflator));

  // sender
  RootLayer layer(root_cid);

  // setup the socket
  ola::network::UdpSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(socket.Bind(ACN_PORT));
  CPPUNIT_ASSERT(socket.EnableBroadcast());

  IncomingUDPTransport incoming_udp_transport(&socket, &root_inflator);
  socket.SetOnData(NewCallback(&incoming_udp_transport,
                               &IncomingUDPTransport::Receive));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(&socket));

  // outgoing transport
  IPV4Address addr;
  CPPUNIT_ASSERT(IPV4Address::FromString("255.255.255.255", &addr));

  OutgoingUDPTransportImpl udp_transport_impl(&socket);
  OutgoingUDPTransport outgoing_udp_transport(&udp_transport_impl, addr);

  // now actually send some data
  MockPDU mock_pdu(4, 8);

  if (root_cid == send_cid)
    CPPUNIT_ASSERT(layer.SendPDU(MockPDU::TEST_VECTOR,
                                 mock_pdu,
                                 &outgoing_udp_transport));
  else
    CPPUNIT_ASSERT(layer.SendPDU(MockPDU::TEST_VECTOR,
                                 mock_pdu,
                                 send_cid,
                                 &outgoing_udp_transport));

  SingleUseCallback0<void> *closure =
    NewSingleCallback(this, &RootLayerTest::FatalStop);
  m_ss->RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS, closure);
  m_ss->Run();
}
}  // e131
}  // plugin
}  // ola
