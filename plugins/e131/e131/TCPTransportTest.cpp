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
#include "ola/io/BufferedWriteDescriptor.h"
#include "ola/io/SelectServer.h"
#include "plugins/e131/e131/PDUTestCommon.h"
#include "plugins/e131/e131/TCPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {

using std::auto_ptr;

class TCPTransportTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TCPTransportTest);
  CPPUNIT_TEST(testSinglePDU);
  CPPUNIT_TEST(testShortPreamble);
  CPPUNIT_TEST(testBadPreamble);
  CPPUNIT_TEST(testZeroLengthPDUBlock);
  CPPUNIT_TEST(testMultiplePDUs);
  CPPUNIT_TEST(testSinglePDUBlock);
  CPPUNIT_TEST(testBufferExpansion);
  CPPUNIT_TEST_SUITE_END();

  public:
    TCPTransportTest(): TestFixture(), m_ss(NULL) {}
    void testSinglePDU();
    void testShortPreamble();
    void testBadPreamble();
    void testZeroLengthPDUBlock();
    void testMultiplePDUs();
    void testMultiplePDUsWithExtraData();
    void testSinglePDUBlock();
    void testBufferExpansion();
    void setUp();

    void Stop();
    void FatalStop() { CPPUNIT_ASSERT(false); }
    void PDUReceived() { m_pdus_received++; }
    void Receive();

  private:
    unsigned int m_pdus_received;
    bool m_stream_ok;
    ola::network::IPV4Address m_localhost;
    auto_ptr<ola::io::SelectServer> m_ss;
    ola::io::BufferedLoopbackDescriptor m_loopback;
    CID m_cid;
    auto_ptr<Callback0<void> > m_rx_callback;
    auto_ptr<MockInflator> m_inflator;
    auto_ptr<IncommingStreamTransport> m_transport;

    void SendEmptyPDUBLock(unsigned int line);
    void SendPDU(unsigned int line);
    void SendPDUBlock(unsigned int line);

    static const int ABORT_TIMEOUT_IN_MS = 1000;
};

CPPUNIT_TEST_SUITE_REGISTRATION(TCPTransportTest);

void TCPTransportTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
  m_stream_ok = true;

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
  m_ss.reset(new ola::io::SelectServer());
  m_ss->RegisterSingleTimeout(
      ABORT_TIMEOUT_IN_MS,
      NewSingleCallback(this, &TCPTransportTest::FatalStop));

  // loopback descriptor
  m_loopback.AssociateSelectServer(m_ss.get());
  CPPUNIT_ASSERT(m_loopback.Init());
  m_loopback.SetOnClose(NewSingleCallback(this, &TCPTransportTest::Stop));
  m_loopback.SetOnData(
      NewCallback(this, &TCPTransportTest::Receive));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(&m_loopback));
}


/**
 * Called when the descriptor is closed.
 */
void TCPTransportTest::Stop() {
  if (m_ss.get())
    m_ss->Terminate();
}


/**
 * Receive data and terminate if the stream is bad.
 */
void TCPTransportTest::Receive() {
  m_stream_ok = m_transport->Receive();
  if (!m_stream_ok)
    m_ss->Terminate();
}

/*
 * Send a single PDU.
 */
void TCPTransportTest::testSinglePDU() {
  SendPDU(__LINE__);
  m_ss->RunOnce(1, 0);
  m_loopback.CloseClient();
  m_ss->RunOnce(1, 0);
  CPPUNIT_ASSERT(m_stream_ok);
  CPPUNIT_ASSERT_EQUAL(1u, m_pdus_received);
}


/**
 * Test a short preamble.
 */
void TCPTransportTest::testShortPreamble() {
  uint8_t bogus_data[] = {
    1, 2, 3, 4,
    1, 2, 3, 4};
  m_loopback.Send(bogus_data, sizeof(bogus_data));

  m_ss->RunOnce(1, 0);
  m_loopback.CloseClient();
  m_ss->RunOnce(1, 0);
  CPPUNIT_ASSERT(m_stream_ok);
  CPPUNIT_ASSERT_EQUAL(0u, m_pdus_received);
}


/**
 * Test bogus data, this should show up as an invalid stream
 */
void TCPTransportTest::testBadPreamble() {
  uint8_t bogus_data[] = {
    1, 2, 3, 4,
    5, 0, 1, 0,
    0, 4, 5, 6,
    7, 8, 9, 0,
    1, 2, 3, 4};
  m_loopback.Send(bogus_data, sizeof(bogus_data));

  m_ss->RunOnce(1, 0);
  m_loopback.CloseClient();
  m_ss->RunOnce(1, 0);
  CPPUNIT_ASSERT(!m_stream_ok);
  CPPUNIT_ASSERT_EQUAL(0u, m_pdus_received);
}


/**
 * Test a 0-length PDU block
 */
void TCPTransportTest::testZeroLengthPDUBlock() {
  SendEmptyPDUBLock(__LINE__);
  SendPDU(__LINE__);

  m_ss->RunOnce(1, 0);
  m_loopback.CloseClient();
  m_ss->RunOnce(1, 0);
  CPPUNIT_ASSERT(m_stream_ok);
  CPPUNIT_ASSERT_EQUAL(1u, m_pdus_received);
}


/**
 * Send multiple PDUs.
 */
void TCPTransportTest::testMultiplePDUs() {
  SendPDU(__LINE__);
  SendPDU(__LINE__);
  SendPDU(__LINE__);

  m_ss->RunOnce(1, 0);
  m_loopback.CloseClient();
  m_ss->RunOnce(1, 0);
  CPPUNIT_ASSERT(m_stream_ok);
  CPPUNIT_ASSERT_EQUAL(3u, m_pdus_received);
}


/**
 * Send a block of PDUs
 */
void TCPTransportTest::testSinglePDUBlock() {
  SendPDUBlock(__LINE__);

  m_ss->RunOnce(1, 0);
  m_loopback.CloseClient();
  m_ss->RunOnce(1, 0);
  CPPUNIT_ASSERT(m_stream_ok);
  CPPUNIT_ASSERT_EQUAL(3u, m_pdus_received);
}


/**
 * Test that we expand the buffer correctly when required
 */
void TCPTransportTest::testBufferExpansion() {
  OutgoingStreamTransport outgoing_transport(&m_loopback);

  // first send a single PDU
  PDUBlock<PDU> pdu_block;
  MockPDU mock_pdu(4, 8);
  pdu_block.AddPDU(&mock_pdu);
  CPPUNIT_ASSERT(outgoing_transport.Send(pdu_block));

  // now follow up with a block
  pdu_block.Clear();
  MockPDU mock_pdu1(1, 2);
  MockPDU mock_pdu2(2, 4);
  MockPDU mock_pdu3(3, 6);
  pdu_block.AddPDU(&mock_pdu1);
  pdu_block.AddPDU(&mock_pdu2);
  pdu_block.AddPDU(&mock_pdu3);

  CPPUNIT_ASSERT(outgoing_transport.Send(pdu_block));

  m_ss->RunOnce(1, 0);
  m_loopback.CloseClient();
  m_ss->RunOnce(1, 0);
  CPPUNIT_ASSERT(m_stream_ok);
  CPPUNIT_ASSERT_EQUAL(4u, m_pdus_received);
}


/**
 * Send empty PDU block.
 */
void TCPTransportTest::SendEmptyPDUBLock(unsigned int line) {
  std::stringstream str;
  str << "Line " << line;
  OutgoingStreamTransport outgoing_transport(&m_loopback);

  // now actually send some data
  PDUBlock<PDU> pdu_block;
  CPPUNIT_ASSERT_MESSAGE(str.str(), outgoing_transport.Send(pdu_block));
}


/**
 * Send a PDU
 */
void TCPTransportTest::SendPDU(unsigned int line) {
  std::stringstream str;
  str << "Line " << line;
  OutgoingStreamTransport outgoing_transport(&m_loopback);

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
  OutgoingStreamTransport outgoing_transport(&m_loopback);

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
