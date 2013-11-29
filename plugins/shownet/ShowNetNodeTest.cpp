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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ShowNetNodeTest.cpp
 * Test fixture for the ShowNetNode class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>

#include "ola/BaseTypes.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "plugins/shownet/ShowNetNode.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace plugin {
namespace shownet {

using ola::DmxBuffer;
using std::map;

class ShowNetNodeTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ShowNetNodeTest);
  CPPUNIT_TEST(testHandlePacket);
  CPPUNIT_TEST(testPopulatePacket);
  CPPUNIT_TEST(testSendAndReceive);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();
    void tearDown();
    void testHandlePacket();
    void testPopulatePacket();
    void testSendAndReceive();
    void UpdateData(unsigned int universe);
    void SendAndReceiveForUniverse(unsigned int universe);
 private:
    bool m_hander_called;
    ShowNetNode *m_node;
};


CPPUNIT_TEST_SUITE_REGISTRATION(ShowNetNodeTest);


void ShowNetNodeTest::setUp() {
  m_node = new ShowNetNode("");
  m_hander_called = false;
}


/*
 * clean up
 */
void ShowNetNodeTest::tearDown() {
  delete m_node;
}


/*
 * Called when there is new data
 */
void ShowNetNodeTest::UpdateData(unsigned int universe) {
  m_hander_called = true;
  (void) universe;
}


/*
 * Test the packet handling code
 */
void ShowNetNodeTest::testHandlePacket() {
  unsigned int universe = 0;
  shownet_data_packet packet;
  unsigned int header_size = sizeof(packet) - sizeof(packet.data);
  const uint8_t ENCODED_DATA[] = {4, 1, 2, 4, 3};
  const uint8_t EXPECTED_DATA[] = {1, 2, 4, 3};
  DmxBuffer expected_dmx(EXPECTED_DATA, sizeof(EXPECTED_DATA));
  memset(&packet, 0, sizeof(packet));
  memcpy(packet.data, ENCODED_DATA, sizeof(ENCODED_DATA));
  DmxBuffer received_data;

  m_node->SetHandler(universe,
                     &received_data,
                     ola::NewCallback(this, &ShowNetNodeTest::UpdateData,
                                     universe));

  // short packets
  OLA_ASSERT_EQ(false, m_node->HandlePacket(packet, 0));
  OLA_ASSERT_FALSE(m_hander_called);
  OLA_ASSERT_EQ(false, m_node->HandlePacket(packet, 5));
  OLA_ASSERT_FALSE(m_hander_called);

  // invalid header
  OLA_ASSERT_EQ(false, m_node->HandlePacket(packet, sizeof(packet)));
  OLA_ASSERT_FALSE(m_hander_called);

  // add a header
  packet.sigHi = ShowNetNode::SHOWNET_ID_HIGH;
  packet.sigLo = ShowNetNode::SHOWNET_ID_LOW;
  OLA_ASSERT_EQ(false, m_node->HandlePacket(packet, sizeof(packet)));
  OLA_ASSERT_FALSE(m_hander_called);

  // add invalid indexBlocks
  packet.indexBlock[0] = 4;
  OLA_ASSERT_EQ(false, m_node->HandlePacket(packet, sizeof(packet)));
  OLA_ASSERT_FALSE(m_hander_called);

  // invalid block length
  packet.indexBlock[0] = ShowNetNode::MAGIC_INDEX_OFFSET;
  OLA_ASSERT_EQ(false, m_node->HandlePacket(packet, sizeof(packet)));
  OLA_ASSERT_FALSE(m_hander_called);

  // add a valid netslot
  packet.netSlot[0] = 1;  // universe 0
  OLA_ASSERT_EQ(false, m_node->HandlePacket(packet, sizeof(packet)));
  OLA_ASSERT_FALSE(m_hander_called);

  // valid block length, but not enough data
  packet.indexBlock[1] = ShowNetNode::MAGIC_INDEX_OFFSET;
  OLA_ASSERT_EQ(false,
                m_node->HandlePacket(packet,
                header_size + sizeof(ENCODED_DATA)));
  OLA_ASSERT_FALSE(m_hander_called);

  // now do a block length larger than the packet
  packet.indexBlock[1] = 100 + ShowNetNode::MAGIC_INDEX_OFFSET;
  OLA_ASSERT_EQ(false,
      m_node->HandlePacket(packet, header_size + sizeof(ENCODED_DATA)));
  OLA_ASSERT_FALSE(m_hander_called);

  // test invalid slot size
  packet.indexBlock[1] = (ShowNetNode::MAGIC_INDEX_OFFSET +
      sizeof(ENCODED_DATA));

  OLA_ASSERT_EQ(false,
      m_node->HandlePacket(packet, header_size + sizeof(ENCODED_DATA)));
  OLA_ASSERT_FALSE(m_hander_called);

  // check a valid packet, but different universe
  packet.netSlot[0] = 513;  // universe 1
  packet.slotSize[0] = sizeof(EXPECTED_DATA);
  OLA_ASSERT_EQ(false,
      m_node->HandlePacket(packet, header_size + sizeof(ENCODED_DATA)));
  OLA_ASSERT_FALSE(m_hander_called);

  // now check with the correct universe
  packet.netSlot[0] = 1;  // universe 0
  OLA_ASSERT_EQ(true,
      m_node->HandlePacket(packet, header_size + sizeof(ENCODED_DATA)));
  OLA_ASSERT_TRUE(m_hander_called);
  OLA_ASSERT_EQ(0,
      memcmp(expected_dmx.GetRaw(), received_data.GetRaw(),
             expected_dmx.Size()));
}


/*
 * Check the packet construction code
 */
void ShowNetNodeTest::testPopulatePacket() {
  const string NAME = "foobarbaz";
  const string DMX_DATA = "abc";

  DmxBuffer buffer(DMX_DATA);
  shownet_data_packet packet;
  shownet_data_packet expected_packet;
  memset(&expected_packet, 0, sizeof(expected_packet));
  unsigned int universe = 0;
  unsigned int header_size = sizeof(packet) - sizeof(packet.data);
  unsigned int encoded_data_size = sizeof(expected_packet.data);
  ola::dmx::RunLengthEncoder encoder;
  encoder.Encode(buffer, expected_packet.data, encoded_data_size);

  m_node->SetName(NAME);
  unsigned int size = m_node->PopulatePacket(&packet, universe, buffer);
  OLA_ASSERT_EQ(header_size + encoded_data_size, size);

  expected_packet.sigHi = ShowNetNode::SHOWNET_ID_HIGH;
  expected_packet.sigLo = ShowNetNode::SHOWNET_ID_LOW;

  OLA_ASSERT_EQ(expected_packet.sigHi, packet.sigHi);
  OLA_ASSERT_EQ(expected_packet.sigLo, packet.sigLo);
  OLA_ASSERT_FALSE(memcmp(expected_packet.ip, packet.ip, sizeof(packet.ip)));

  expected_packet.netSlot[0] = 1;
  expected_packet.slotSize[0] = DMX_DATA.length();
  expected_packet.indexBlock[0] = ShowNetNode::MAGIC_INDEX_OFFSET;
  expected_packet.indexBlock[1] = (encoded_data_size +
     ShowNetNode::MAGIC_INDEX_OFFSET);

  OLA_ASSERT_FALSE(memcmp(expected_packet.netSlot,
                         packet.netSlot,
                         sizeof(packet.netSlot)));
  OLA_ASSERT_FALSE(memcmp(expected_packet.slotSize,
                         packet.slotSize,
                         sizeof(packet.slotSize)));
  OLA_ASSERT_FALSE(memcmp(expected_packet.indexBlock,
                         packet.indexBlock,
                         sizeof(packet.indexBlock)));

  OLA_ASSERT_EQ(expected_packet.packetCountHi, packet.packetCountHi);
  OLA_ASSERT_EQ(expected_packet.packetCountLo, packet.packetCountLo);

  OLA_ASSERT_FALSE(memcmp(expected_packet.block,
                         packet.block,
                         sizeof(packet.block)));
  memcpy(expected_packet.name, NAME.data(), sizeof(expected_packet.name));
  OLA_ASSERT_FALSE(memcmp(expected_packet.name,
                         packet.name,
                         sizeof(packet.name)));

  OLA_ASSERT_FALSE(memcmp(expected_packet.data,
                         packet.data,
                        encoded_data_size));
  OLA_ASSERT_FALSE(memcmp(&expected_packet, &packet, size));

  // now send for a different universe
  universe = 1;
  size = m_node->PopulatePacket(&packet, universe, buffer);
  expected_packet.netSlot[0] = 513;
  OLA_ASSERT_FALSE(memcmp(&expected_packet, &packet, size));
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
  shownet_data_packet packet;
  DmxBuffer received_data;

  m_node->SetHandler(
      universe,
      &received_data,
      ola::NewCallback(this, &ShowNetNodeTest::UpdateData, universe));

  // zero first
  size = m_node->PopulatePacket(&packet, universe, zero_buffer);
  m_node->HandlePacket(packet, size);
  OLA_ASSERT(received_data == zero_buffer);

  // send a test packet
  size = m_node->PopulatePacket(&packet, universe, buffer1);
  m_node->HandlePacket(packet, size);
  OLA_ASSERT_EQ(
      0,
      memcmp(buffer1.GetRaw(), received_data.GetRaw(), buffer1.Size()));

  // send another test packet
  size = m_node->PopulatePacket(&packet, universe, buffer2);
  m_node->HandlePacket(packet, size);
  OLA_ASSERT_EQ(
      0,
      memcmp(buffer2.GetRaw(), received_data.GetRaw(), buffer2.Size()));

  // check that we don't mix up universes
  size = m_node->PopulatePacket(&packet, universe + 1, buffer1);
  m_node->HandlePacket(packet, size);
  OLA_ASSERT_EQ(
      0,
      memcmp(buffer2.GetRaw(), received_data.GetRaw(), buffer2.Size()));
}
}  // namespace shownet
}  // namespace plugin
}  // namespace ola
