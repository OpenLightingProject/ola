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
 * TCPTransportTest.cpp
 * Test fixture for the TCPTransport class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>

#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/IOStack.h"
#include "ola/io/SelectServer.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/PreamblePacker.h"
#include "libs/acn/TCPTransport.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace acn {

using ola::TimeInterval;
using ola::acn::CID;
using ola::io::IOQueue;
using ola::io::IOStack;
using ola::network::IPV4SocketAddress;
using std::auto_ptr;
using std::string;

class TCPTransportTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TCPTransportTest);
  CPPUNIT_TEST(testSinglePDU);
  CPPUNIT_TEST(testShortPreamble);
  CPPUNIT_TEST(testBadPreamble);
  CPPUNIT_TEST(testZeroLengthPDUBlock);
  CPPUNIT_TEST(testMultiplePDUs);
  CPPUNIT_TEST(testSinglePDUBlock);
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
    void setUp();
    void tearDown();

    void Stop();
    void FatalStop() { OLA_ASSERT(false); }
    void PDUReceived() { m_pdus_received++; }
    void Receive();

 private:
    unsigned int m_pdus_received;
    bool m_stream_ok;
    ola::network::IPV4SocketAddress m_localhost;
    auto_ptr<ola::io::SelectServer> m_ss;
    ola::io::LoopbackDescriptor m_loopback;
    CID m_cid;
    auto_ptr<Callback0<void> > m_rx_callback;
    auto_ptr<MockInflator> m_inflator;
    auto_ptr<IncomingStreamTransport> m_transport;

    void SendEmptyPDUBLock(const ola::testing::SourceLine &source_line);
    void SendPDU(const ola::testing::SourceLine &source_line);
    void SendPDUBlock(const ola::testing::SourceLine &source_line);
    void SendPacket(const ola::testing::SourceLine &source_line,
                    IOStack *packet);

    static const int ABORT_TIMEOUT_IN_MS = 1000;
};

CPPUNIT_TEST_SUITE_REGISTRATION(TCPTransportTest);

void TCPTransportTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
  m_stream_ok = true;
  m_pdus_received = 0;

  m_localhost = IPV4SocketAddress::FromStringOrDie("127.0.0.1:9999");

  // mock inflator
  CID cid;
  m_rx_callback.reset(NewCallback(this, &TCPTransportTest::PDUReceived));
  m_inflator.reset(new MockInflator(m_cid, m_rx_callback.get()));

  // transport to test
  m_transport.reset(
      new IncomingStreamTransport(m_inflator.get(), &m_loopback, m_localhost));

  // SelectServer
  m_ss.reset(new ola::io::SelectServer());
  m_ss->RegisterSingleTimeout(
      ABORT_TIMEOUT_IN_MS,
      NewSingleCallback(this, &TCPTransportTest::FatalStop));

  // loopback descriptor
  OLA_ASSERT(m_loopback.Init());
  m_loopback.SetOnClose(NewSingleCallback(this, &TCPTransportTest::Stop));
  m_loopback.SetOnData(
      NewCallback(this, &TCPTransportTest::Receive));
  OLA_ASSERT(m_ss->AddReadDescriptor(&m_loopback));
}


void TCPTransportTest::tearDown() {
  // Close the loopback descriptor and drain the ss
  m_loopback.Close();
  m_ss->RunOnce();
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
  OLA_ASSERT_EQ(0u, m_pdus_received);
  SendPDU(OLA_SOURCELINE());
  m_ss->RunOnce(TimeInterval(1, 0));
  m_loopback.CloseClient();
  m_ss->RunOnce(TimeInterval(1, 0));
  OLA_ASSERT(m_stream_ok);
  OLA_ASSERT_EQ(1u, m_pdus_received);
}


/**
 * Test a short preamble.
 */
void TCPTransportTest::testShortPreamble() {
  uint8_t bogus_data[] = {
    1, 2, 3, 4,
    1, 2, 3, 4};
  m_loopback.Send(bogus_data, sizeof(bogus_data));

  m_ss->RunOnce(TimeInterval(1, 0));
  m_loopback.CloseClient();
  m_ss->RunOnce(TimeInterval(1, 0));
  OLA_ASSERT(m_stream_ok);
  OLA_ASSERT_EQ(0u, m_pdus_received);
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

  m_ss->RunOnce(TimeInterval(1, 0));
  m_loopback.CloseClient();
  m_ss->RunOnce(TimeInterval(1, 0));
  OLA_ASSERT_FALSE(m_stream_ok);
  OLA_ASSERT_EQ(0u, m_pdus_received);
}


/**
 * Test a 0-length PDU block
 */
void TCPTransportTest::testZeroLengthPDUBlock() {
  SendEmptyPDUBLock(OLA_SOURCELINE());
  SendPDU(OLA_SOURCELINE());

  m_ss->RunOnce(TimeInterval(1, 0));
  m_loopback.CloseClient();
  m_ss->RunOnce(TimeInterval(1, 0));
  OLA_ASSERT(m_stream_ok);
  OLA_ASSERT_EQ(1u, m_pdus_received);
}


/**
 * Send multiple PDUs.
 */
void TCPTransportTest::testMultiplePDUs() {
  SendPDU(OLA_SOURCELINE());
  SendPDU(OLA_SOURCELINE());
  SendPDU(OLA_SOURCELINE());

  m_ss->RunOnce(TimeInterval(1, 0));
  m_loopback.CloseClient();
  m_ss->RunOnce(TimeInterval(1, 0));
  OLA_ASSERT(m_stream_ok);
  OLA_ASSERT_EQ(3u, m_pdus_received);
}


/**
 * Send a block of PDUs
 */
void TCPTransportTest::testSinglePDUBlock() {
  SendPDUBlock(OLA_SOURCELINE());

  m_ss->RunOnce(TimeInterval(1, 0));
  m_loopback.CloseClient();
  m_ss->RunOnce(TimeInterval(1, 0));
  OLA_ASSERT(m_stream_ok);
  OLA_ASSERT_EQ(3u, m_pdus_received);
}


/**
 * Send empty PDU block.
 */
void TCPTransportTest::SendEmptyPDUBLock(
    const ola::testing::SourceLine &source_line) {
  IOStack packet;
  PreamblePacker::AddTCPPreamble(&packet);
  SendPacket(source_line, &packet);
}


/**
 * Send a PDU
 */
void TCPTransportTest::SendPDU(const ola::testing::SourceLine &source_line) {
  IOStack packet;
  MockPDU::PrependPDU(&packet, 4, 8);
  PreamblePacker::AddTCPPreamble(&packet);
  SendPacket(source_line, &packet);
}


/**
 * Send a block of PDUs
 */
void TCPTransportTest::SendPDUBlock(
    const ola::testing::SourceLine &source_line) {
  IOStack packet;
  MockPDU::PrependPDU(&packet, 1, 2);
  MockPDU::PrependPDU(&packet, 2, 4);
  MockPDU::PrependPDU(&packet, 3, 6);
  PreamblePacker::AddTCPPreamble(&packet);
  SendPacket(source_line, &packet);
}


void TCPTransportTest::SendPacket(const ola::testing::SourceLine &source_line,
                                  IOStack *packet) {
  IOQueue output;
  packet->MoveToIOQueue(&output);
  ola::testing::_FailIf(
      source_line,
      !m_loopback.Send(&output),
      "Loopback send failed");
}
}  // namespace acn
}  // namespace ola
