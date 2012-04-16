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
 * TCPTransportTest.cpp
 * Test fixture for the TCPTransport class
 * Copyright (C) 2012 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/Logging.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "plugins/e131/e131/PDUTestCommon.h"
#include "plugins/e131/e131/TCPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {

using std::auto_ptr;

class TCPTransportTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TCPTransportTest);
  CPPUNIT_TEST(testSinglePDU);
  CPPUNIT_TEST(testSinglePDUWithExtraData);
  CPPUNIT_TEST(testMultiplePDUs);
  CPPUNIT_TEST(testMultiplePDUsWithExtraData);
  // TODO(simon): fix this.
  // This isn't supported by the current Transport
  // We're waiting for resolution on RLP over TCP
  // CPPUNIT_TEST(testSinglePDUBlock);
  CPPUNIT_TEST_SUITE_END();

  public:
    TCPTransportTest(): TestFixture(), m_ss(NULL) {}
    void testSinglePDU();
    void testSinglePDUWithExtraData();
    void testMultiplePDUs();
    void testMultiplePDUsWithExtraData();
    void testSinglePDUBlock();
    void setUp();

    void Stop();
    void FatalStop() { CPPUNIT_ASSERT(false); }
    void PDUReceived() { m_pdus_received++; }

  private:
    unsigned int m_pdus_received;
    ola::network::IPV4Address m_localhost;
    auto_ptr<ola::network::SelectServer> m_ss;
    ola::network::LoopbackDescriptor m_loopback;
    CID m_cid;
    auto_ptr<Callback0<void> > m_rx_callback;
    auto_ptr<MockInflator> m_inflator;
    auto_ptr<IncommingStreamTransport> m_transport;

    void SendPDU(unsigned int line);
    void SendPDUBlock(unsigned int line);

    static const int ABORT_TIMEOUT_IN_MS = 1000;
};

CPPUNIT_TEST_SUITE_REGISTRATION(TCPTransportTest);

void TCPTransportTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
  m_pdus_received = 0;

  CPPUNIT_ASSERT(IPV4Address::FromString("127.0.0.1", &m_localhost));

  // mock inflator
  CID cid;
  m_rx_callback.reset(NewCallback(this, &TCPTransportTest::PDUReceived));
  m_inflator.reset(new MockInflator(m_cid, m_rx_callback.get()));

  // transport to test
  m_transport.reset(
      new IncommingStreamTransport(m_inflator.get(),
                                   &m_loopback,
                                   m_localhost,
                                   9999));

  // SelectServer
  m_ss.reset(new ola::network::SelectServer());
  m_ss->RegisterSingleTimeout(
      ABORT_TIMEOUT_IN_MS,
      NewSingleCallback(this, &TCPTransportTest::FatalStop));

  // loopback descriptor
  CPPUNIT_ASSERT(m_loopback.Init());
  m_loopback.SetOnClose(NewSingleCallback(this, &TCPTransportTest::Stop));
  m_loopback.SetOnData(
      NewCallback(m_transport.get(), &IncommingStreamTransport::Receive));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(&m_loopback));
}


/**
 * Called when the descriptor is closed.
 */
void TCPTransportTest::Stop() {
  if (m_ss.get())
    m_ss->Terminate();
}


/*
 * Send a single PDU.
 */
void TCPTransportTest::testSinglePDU() {
  SendPDU(__LINE__);
  m_loopback.CloseClient();
  m_ss->Run();
  CPPUNIT_ASSERT_EQUAL(1u, m_pdus_received);
}


/**
 * Send some bogus data, followed by a PDU.
 */
void TCPTransportTest::testSinglePDUWithExtraData() {
  uint8_t bogus_data[] = {1, 2, 3, 4, 5, 0, 0x10, 0, 0, 0x41, 0x53, 0x43};
  m_loopback.Send(bogus_data, sizeof(bogus_data));
  SendPDU(__LINE__);

  m_loopback.CloseClient();
  m_ss->Run();
  CPPUNIT_ASSERT_EQUAL(1u, m_pdus_received);
}


/**
 * Send multiple PDUs.
 */
void TCPTransportTest::testMultiplePDUs() {
  SendPDU(__LINE__);
  SendPDU(__LINE__);
  SendPDU(__LINE__);

  m_loopback.CloseClient();
  m_ss->Run();
  CPPUNIT_ASSERT_EQUAL(3u, m_pdus_received);
}


/**
 * Send multiple PDUs with extra data between them
 */
void TCPTransportTest::testMultiplePDUsWithExtraData() {
  uint8_t bogus_data[] = {1, 2, 3, 4, 5, 0, 0x10, 0, 0, 0x41, 0x53, 0x43};
  unsigned int PDU_LIMIT = 20;

  for (unsigned int i = 0; i < PDU_LIMIT; i++) {
    m_loopback.Send(bogus_data, sizeof(bogus_data));
    SendPDU(__LINE__);
  }
  m_loopback.Send(bogus_data, sizeof(bogus_data));

  m_loopback.CloseClient();
  m_ss->Run();
  CPPUNIT_ASSERT_EQUAL(PDU_LIMIT, m_pdus_received);
}


/**
 * Send a block of PDUs
 */
void TCPTransportTest::testSinglePDUBlock() {
  SendPDUBlock(__LINE__);

  m_loopback.CloseClient();
  m_ss->Run();
  CPPUNIT_ASSERT_EQUAL(3u, m_pdus_received);
}


/**
 * Send a PDU
 */
void TCPTransportTest::SendPDU(unsigned int line) {
  std::stringstream str;
  str << "Line " << line;
  TCPTransport outgoing_transport(&m_loopback);

  // now actually send some data
  PDUBlock<PDU> pdu_block;
  MockPDU mock_pdu(4, 8);
  pdu_block.AddPDU(&mock_pdu);
  CPPUNIT_ASSERT_MESSAGE(str.str(), outgoing_transport.Send(pdu_block));
}


/**
 * Send a block of PDUs
 */
void TCPTransportTest::SendPDUBlock(unsigned int line) {
  std::stringstream str;
  str << "Line " << line;
  TCPTransport outgoing_transport(&m_loopback);

  // now actually send some data
  PDUBlock<PDU> pdu_block;
  MockPDU mock_pdu1(1, 2);
  MockPDU mock_pdu2(2, 4);
  MockPDU mock_pdu3(3, 6);

  pdu_block.AddPDU(&mock_pdu1);
  pdu_block.AddPDU(&mock_pdu2);
  pdu_block.AddPDU(&mock_pdu3);
  CPPUNIT_ASSERT_MESSAGE(str.str(), outgoing_transport.Send(pdu_block));
}
}  // e131
}  // plugin
}  // ola
