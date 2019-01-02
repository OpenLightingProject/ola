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
 * ShowNetNodeTest.cpp
 * Test fixture for the ShowNetNode class
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>

#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"
#include "plugins/shownet/ShowNetNode.h"

namespace ola {
namespace plugin {
namespace shownet {

using ola::DmxBuffer;
using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using std::string;

namespace {

// HostToLittleEndian is overloaded, so to avoid lots of casts we provide this
// function.
uint16_t ToLittleEndian(uint16_t value) {
  return ola::network::HostToLittleEndian(value);
}
}  // namespace

class ShowNetNodeTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ShowNetNodeTest);
  CPPUNIT_TEST(testHandlePacket);
  CPPUNIT_TEST(testExtractPacket);
  CPPUNIT_TEST(testPopulatePacket);
  CPPUNIT_TEST(testSendAndReceive);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();

    void testHandlePacket();
    void testExtractPacket();
    void testPopulatePacket();
    void testSendAndReceive();
    void UpdateData(unsigned int universe);
    void SendAndReceiveForUniverse(unsigned int universe);
 private:
    int m_handler_called;
    std::auto_ptr<ShowNetNode> m_node;

    static const uint8_t EXPECTED_PACKET[];
    static const uint8_t EXPECTED_PACKET2[];
};

CPPUNIT_TEST_SUITE_REGISTRATION(ShowNetNodeTest);

// Start slot 1
const uint8_t ShowNetNodeTest::EXPECTED_PACKET[] = {
  0x80, 0x8f, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0,  // net slots
  3, 0, 0, 0, 0, 0, 0, 0,  // slot sizes
  11, 0, 15, 0, 0, 0, 0, 0, 0, 0,  // index blocks
  0, 0, 0, 0, 0, 0,
  'f', 'o', 'o', 'b', 'a', 'r', 'b', 'a', 'z',
  3, 'a', 'b', 'c',
};

// Start slot 513
const uint8_t ShowNetNodeTest::EXPECTED_PACKET2[] = {
  0x80, 0x8f, 0, 0, 0, 0,
  1, 2, 0, 0, 0, 0, 0, 0,  // net slots
  3, 0, 0, 0, 0, 0, 0, 0,  // slot sizes
  11, 0, 15, 0, 0, 0, 0, 0, 0, 0,  // index blocks
  0, 0, 0, 0, 0, 0,
  'f', 'o', 'o', 'b', 'a', 'r', 'b', 'a', 'z',
  3, 'a', 'b', 'c',
};

void ShowNetNodeTest::setUp() {
  m_handler_called = 0;
  // We pass in a blank interface here, so the uninitialised IP (0.0.0.0)
  // matches the content of our expected packets
  ola::network::Interface iface;
  m_node.reset(new ShowNetNode(iface));
}

/*
 * Called when there is new data
 */
void ShowNetNodeTest::UpdateData(unsigned int) {
  m_handler_called++;
}

/*
 * Test the packet handling code
 */
void ShowNetNodeTest::testHandlePacket() {
  unsigned int universe = 0;
  const uint8_t ENCODED_DATA[] = {4, 1, 2, 4, 3};
  const uint8_t EXPECTED_DATA[] = {1, 2, 4, 3};
  DmxBuffer expected_dmx(EXPECTED_DATA, sizeof(EXPECTED_DATA));

  shownet_packet packet;
  shownet_compressed_dmx *compressed_dmx = &packet.data.compressed_dmx;
  memset(&packet, 0, sizeof(packet));
  memcpy(compressed_dmx->data, ENCODED_DATA, sizeof(ENCODED_DATA));
  DmxBuffer received_data;

  m_node->SetHandler(
      universe, &received_data,
      ola::NewCallback(this, &ShowNetNodeTest::UpdateData, universe));

  // short packets
  OLA_ASSERT_EQ(false, m_node->HandlePacket(&packet, 0));
  OLA_ASSERT_EQ(0, m_handler_called);
  OLA_ASSERT_EQ(false, m_node->HandlePacket(&packet, 5));
  OLA_ASSERT_EQ(0, m_handler_called);

  // invalid header
  OLA_ASSERT_EQ(false, m_node->HandlePacket(&packet, sizeof(packet)));
  OLA_ASSERT_EQ(0, m_handler_called);

  // add a header
  packet.type = HostToNetwork(static_cast<uint16_t>(COMPRESSED_DMX_PACKET));
  OLA_ASSERT_EQ(false, m_node->HandlePacket(&packet, sizeof(packet)));
  OLA_ASSERT_EQ(0, m_handler_called);

  // add invalid indexBlocks
  compressed_dmx->indexBlock[0] = ToLittleEndian(4);
  OLA_ASSERT_EQ(false, m_node->HandlePacket(&packet, sizeof(packet)));
  OLA_ASSERT_EQ(0, m_handler_called);

  // invalid block length
  compressed_dmx->indexBlock[0] =
      ToLittleEndian(ShowNetNode::MAGIC_INDEX_OFFSET);
  OLA_ASSERT_EQ(false, m_node->HandlePacket(&packet, sizeof(packet)));
  OLA_ASSERT_EQ(0, m_handler_called);

  // add a valid netslot
  compressed_dmx->netSlot[0] = ToLittleEndian(1);  // universe 0
  OLA_ASSERT_EQ(false, m_node->HandlePacket(&packet, sizeof(packet)));
  OLA_ASSERT_EQ(0, m_handler_called);

  // valid block length, but not enough data
  unsigned int header_size = sizeof(packet) - sizeof(packet.data);
  compressed_dmx->indexBlock[1] =
      ToLittleEndian(ShowNetNode::MAGIC_INDEX_OFFSET);
  OLA_ASSERT_EQ(false,
                m_node->HandlePacket(&packet,
                header_size + sizeof(ENCODED_DATA)));
  OLA_ASSERT_EQ(0, m_handler_called);

  // now do a block length larger than the packet
  compressed_dmx->indexBlock[1] = ToLittleEndian(
      100 + ShowNetNode::MAGIC_INDEX_OFFSET);
  OLA_ASSERT_EQ(false,
      m_node->HandlePacket(&packet, header_size + sizeof(ENCODED_DATA)));
  OLA_ASSERT_EQ(0, m_handler_called);

  // test invalid slot size
  compressed_dmx->indexBlock[1] = ToLittleEndian(
      ShowNetNode::MAGIC_INDEX_OFFSET + sizeof(ENCODED_DATA));

  OLA_ASSERT_EQ(false,
      m_node->HandlePacket(&packet, header_size + sizeof(ENCODED_DATA)));
  OLA_ASSERT_EQ(0, m_handler_called);

  // check a valid packet, but different universe
  compressed_dmx->netSlot[0] = ToLittleEndian(513);  // universe 1
  compressed_dmx->slotSize[0] = ToLittleEndian(sizeof(EXPECTED_DATA));
  OLA_ASSERT_EQ(false,
      m_node->HandlePacket(&packet, header_size + sizeof(ENCODED_DATA)));
  OLA_ASSERT_EQ(0, m_handler_called);

  // now check with the correct universe
  compressed_dmx->netSlot[0] = ToLittleEndian(1);  // universe 0
  OLA_ASSERT_EQ(true,
      m_node->HandlePacket(&packet, header_size + sizeof(ENCODED_DATA)));
  OLA_ASSERT_EQ(1, m_handler_called);
  OLA_ASSERT_EQ(0,
      memcmp(expected_dmx.GetRaw(), received_data.GetRaw(),
             expected_dmx.Size()));
}


void ShowNetNodeTest::testExtractPacket() {
  unsigned int universe = 10;
  DmxBuffer received_data;
  m_node->SetHandler(
      universe,
      &received_data,
      ola::NewCallback(this, &ShowNetNodeTest::UpdateData, universe));

  // Packets from https://code.google.com/p/open-lighting/issues/detail?id=218
  const uint8_t packet1[] = {
    0x80, 0x8f, 0x01, 0xb6, 0xc0, 0xa8,  // header
    0x01, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // net slots
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // slot sizes
    0x0b, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // index blocks
    0x00, 0x71, 0x00, 0x00, 0x00, 0x04,
    0x49, 0x6e, 0x70, 0x75, 0x74, 0x00, 0x00, 0x00, 0x00,  // name
    0x05, 0x11, 0x4e, 0x32, 0x3c, 0x05, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
    0xfe, 0x00
  };
  DmxBuffer expected_data1;
  expected_data1.Blackout();
  const uint8_t expected_dmx_data1[] = {17, 78, 50, 60, 5};
  expected_data1.SetRange(0, expected_dmx_data1, sizeof(expected_dmx_data1));

  OLA_ASSERT_TRUE(m_node->HandlePacket(
      reinterpret_cast<const shownet_packet*>(packet1), sizeof(packet1)));
  OLA_ASSERT_EQ(1, m_handler_called);
  OLA_ASSERT_DATA_EQUALS(expected_data1.GetRaw(), expected_data1.Size(),
                         received_data.GetRaw(), received_data.Size());
}

/*
 * Check the packet construction code.
 */
void ShowNetNodeTest::testPopulatePacket() {
  unsigned int universe = 0;
  const string NAME = "foobarbaz";
  const string DMX_DATA = "abc";

  DmxBuffer buffer(DMX_DATA);
  shownet_packet packet;
  m_node->SetName(NAME);

  unsigned int size = m_node->BuildCompressedPacket(&packet, universe, buffer);
  OLA_ASSERT_DATA_EQUALS(EXPECTED_PACKET, sizeof(EXPECTED_PACKET),
                         reinterpret_cast<const uint8_t*>(&packet), size);

  // now send for a different universe
  universe = 1;
  size = m_node->BuildCompressedPacket(&packet, universe, buffer);
  OLA_ASSERT_DATA_EQUALS(EXPECTED_PACKET2, sizeof(EXPECTED_PACKET2),
                         reinterpret_cast<const uint8_t*>(&packet), size);
}


/*
 * Check that we can decode the packets we send.
 */
void ShowNetNodeTest::testSendAndReceive() {
  SendAndReceiveForUniverse(0);
  SendAndReceiveForUniverse(1);
  SendAndReceiveForUniverse(2);
}


/*
 * Send and receive some packets on this universe.
 */
void ShowNetNodeTest::SendAndReceiveForUniverse(unsigned int universe) {
  const uint8_t TEST_DATA[] = {1, 2, 2, 3, 0, 0, 0, 1, 3, 3, 3, 1, 2};
  const uint8_t TEST_DATA2[] = {0, 0, 0, 0, 6, 5, 4, 3, 3, 3};

  DmxBuffer zero_buffer;
  zero_buffer.Blackout();
  DmxBuffer buffer1(TEST_DATA, sizeof(TEST_DATA));
  DmxBuffer buffer2(TEST_DATA2, sizeof(TEST_DATA2));
  unsigned int size;
  shownet_packet packet;
  DmxBuffer received_data;

  m_node->SetHandler(
      universe,
      &received_data,
      ola::NewCallback(this, &ShowNetNodeTest::UpdateData, universe));

  // zero first
  size = m_node->BuildCompressedPacket(&packet, universe, zero_buffer);
  m_node->HandlePacket(&packet, size);
  OLA_ASSERT_DMX_EQUALS(received_data, zero_buffer);

  // send a test packet
  size = m_node->BuildCompressedPacket(&packet, universe, buffer1);
  m_node->HandlePacket(&packet, size);
  OLA_ASSERT_EQ(
      0,
      memcmp(buffer1.GetRaw(), received_data.GetRaw(), buffer1.Size()));

  // send another test packet
  size = m_node->BuildCompressedPacket(&packet, universe, buffer2);
  m_node->HandlePacket(&packet, size);
  OLA_ASSERT_EQ(
      0,
      memcmp(buffer2.GetRaw(), received_data.GetRaw(), buffer2.Size()));

  // check that we don't mix up universes
  size = m_node->BuildCompressedPacket(&packet, universe + 1, buffer1);
  m_node->HandlePacket(&packet, size);
  OLA_ASSERT_EQ(
      0,
      memcmp(buffer2.GetRaw(), received_data.GetRaw(), buffer2.Size()));
}
}  // namespace shownet
}  // namespace plugin
}  // namespace ola
