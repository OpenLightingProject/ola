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
 * SPIOutputTest.cpp
 * Test fixture for SPIOutput.
 * Copyright (C) 2013 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/base/Array.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"
#include "plugins/spi/SPIBackend.h"
#include "plugins/spi/SPIOutput.h"

using ola::DmxBuffer;
using ola::plugin::spi::FakeSPIBackend;
using ola::plugin::spi::SPIBackendInterface;
using ola::plugin::spi::SPIOutput;
using ola::rdm::UID;
using std::string;

class SPIOutputTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SPIOutputTest);
  CPPUNIT_TEST(testDescription);
  CPPUNIT_TEST(testIndividualWS2801Control);
  CPPUNIT_TEST(testCombinedWS2801Control);
  CPPUNIT_TEST(testIndividualLPD8806Control);
  CPPUNIT_TEST(testCombinedLPD8806Control);
  CPPUNIT_TEST(testIndividualP9813Control);
  CPPUNIT_TEST(testCombinedP9813Control);
  CPPUNIT_TEST(testIndividualAPA102Control);
  CPPUNIT_TEST(testCombinedAPA102Control);
  CPPUNIT_TEST_SUITE_END();

 public:
  SPIOutputTest()
      : CppUnit::TestFixture(),
        m_uid(0x707a, 0) {
  }
  void setUp();

  void testDescription();
  void testIndividualWS2801Control();
  void testCombinedWS2801Control();
  void testIndividualLPD8806Control();
  void testCombinedLPD8806Control();
  void testIndividualP9813Control();
  void testCombinedP9813Control();
  void testIndividualAPA102Control();
  void testCombinedAPA102Control();

 private:
  UID m_uid;
};


CPPUNIT_TEST_SUITE_REGISTRATION(SPIOutputTest);

void SPIOutputTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
}

/**
 * Check the descrption, start address & personality.
 */
void SPIOutputTest::testDescription() {
  FakeSPIBackend backend(2);
  SPIOutput output1(m_uid, &backend, SPIOutput::Options(0, "Test SPI Device"));
  SPIOutput::Options options(1, "Test SPI Device");
  options.pixel_count = 32;
  SPIOutput output2(m_uid, &backend, options);

  OLA_ASSERT_EQ(
      string("test, output 0, WS2801 Individual Control, 75 slots @ 1."
             " (707a:00000000)"),
      output1.Description());
  OLA_ASSERT_EQ(static_cast<uint16_t>(1), output1.GetStartAddress());
  OLA_ASSERT_EQ(static_cast<uint8_t>(1), output1.GetPersonality());
  OLA_ASSERT_EQ(
      string("test, output 1, WS2801 Individual Control, 96 slots @ 1."
             " (707a:00000000)"),
      output2.Description());

  // change the start address & personality
  output1.SetStartAddress(10);
  output1.SetPersonality(3);
  OLA_ASSERT_EQ(
      string("test, output 0, LPD8806 Individual Control, 75 slots @ 10."
             " (707a:00000000)"),
      output1.Description());
  OLA_ASSERT_EQ(static_cast<uint16_t>(10), output1.GetStartAddress());
  OLA_ASSERT_EQ(static_cast<uint8_t>(3), output1.GetPersonality());
}


/**
 * Test DMX writes in the individual WS2801 mode.
 */
void SPIOutputTest::testIndividualWS2801Control() {
  FakeSPIBackend backend(2);
  SPIOutput::Options options(0, "Test SPI Device");
  options.pixel_count = 2;
  SPIOutput output(m_uid, &backend, options);

  DmxBuffer buffer;
  unsigned int length = 0;
  const uint8_t *data = NULL;

  buffer.SetFromString("1, 10, 100");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED0[] = { 1, 10, 100, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED0, arraysize(EXPECTED0), data, length);
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  buffer.SetFromString("255,128,0,10,20,30");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED1[] = { 255, 128, 0, 10, 20, 30 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 34, 56, 78, 10, 20, 30 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));

  buffer.SetFromString("7, 9");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED3[] = { 7, 9, 78, 10, 20, 30 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED3, arraysize(EXPECTED3), data, length);
  OLA_ASSERT_EQ(4u, backend.Writes(0));

  output.SetStartAddress(3);
  buffer.SetFromString("1,2,3,4,5,6,7,8");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED4[] = { 3, 4, 5, 6, 7, 8 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED4, arraysize(EXPECTED4), data, length);
  OLA_ASSERT_EQ(5u, backend.Writes(0));

  // Check nothing changed on the other output.
  OLA_ASSERT_EQ(reinterpret_cast<const uint8_t*>(NULL),
                backend.GetData(1, &length));
  OLA_ASSERT_EQ(0u, backend.Writes(1));
}


/**
 * Test DMX writes in the combined WS2801 mode.
 */
void SPIOutputTest::testCombinedWS2801Control() {
  FakeSPIBackend backend(2);
  SPIOutput::Options options(0, "Test SPI Device");
  options.pixel_count = 2;
  SPIOutput output(m_uid, &backend, options);
  output.SetPersonality(2);

  DmxBuffer buffer;
  buffer.SetFromString("255,128,0,10,20,30");
  output.WriteDMX(buffer);

  unsigned int length = 0;
  const uint8_t *data = backend.GetData(0, &length);

  const uint8_t EXPECTED1[] = { 255, 128, 0, 255, 128, 0 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 34, 56, 78, 34, 56, 78 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  // Frames with insufficient data don't trigger writes.
  buffer.SetFromString("7, 9");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  output.SetStartAddress(3);
  buffer.SetFromString("1,2,3,4,5,6,7,8");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED4[] = { 3, 4, 5, 3, 4, 5};
  OLA_ASSERT_DATA_EQUALS(EXPECTED4, arraysize(EXPECTED4), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));

  // Check nothing changed on the other output.
  OLA_ASSERT_EQ(reinterpret_cast<const uint8_t*>(NULL),
                backend.GetData(1, &length));
  OLA_ASSERT_EQ(0u, backend.Writes(1));
}

/**
 * Test DMX writes in the individual LPD8806 mode.
 */
void SPIOutputTest::testIndividualLPD8806Control() {
  FakeSPIBackend backend(2);
  SPIOutput::Options options(0, "Test SPI Device");
  options.pixel_count = 2;
  SPIOutput output(m_uid, &backend, options);
  output.SetPersonality(3);

  DmxBuffer buffer;
  unsigned int length = 0;
  const uint8_t *data = NULL;

  buffer.SetFromString("1, 10, 100");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED0[] = { 0x85, 0x80, 0xb2, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED0, arraysize(EXPECTED0), data, length);
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  buffer.SetFromString("255,128,0,10,20,30");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED1[] = { 0xc0, 0xff, 0x80, 0x8a, 0x85, 0x8f, 0 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 0x9c, 0x91, 0xa7, 0x8a, 0x85, 0x8f, 0 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));

  buffer.SetFromString("7, 9");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));

  output.SetStartAddress(3);
  buffer.SetFromString("1,2,3,4,5,6,7,8");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED4[] = { 0x82, 0x81, 0x82, 0x83, 0x83, 0x84, 0 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED4, arraysize(EXPECTED4), data, length);
  OLA_ASSERT_EQ(4u, backend.Writes(0));

  // Check nothing changed on the other output.
  OLA_ASSERT_EQ(reinterpret_cast<const uint8_t*>(NULL),
                backend.GetData(1, &length));
  OLA_ASSERT_EQ(0u, backend.Writes(1));
}

/**
 * Test DMX writes in the combined LPD8806 mode.
 */
void SPIOutputTest::testCombinedLPD8806Control() {
  FakeSPIBackend backend(2);
  SPIOutput::Options options(0, "Test SPI Device");
  options.pixel_count = 2;
  SPIOutput output(m_uid, &backend, options);
  output.SetPersonality(4);

  DmxBuffer buffer;
  buffer.SetFromString("255,128,0,10,20,30");
  output.WriteDMX(buffer);

  unsigned int length = 0;
  const uint8_t *data = backend.GetData(0, &length);

  const uint8_t EXPECTED1[] = { 0xc0, 0xff, 0x80, 0xc0, 0xff, 0x80, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 0x9c, 0x91, 0xa7, 0x9c, 0x91, 0xa7, 0 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  buffer.SetFromString("7, 9");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  output.SetStartAddress(3);
  buffer.SetFromString("1,2,3,4,5,6,7,8");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED4[] = { 0x82, 0x81, 0x82, 0x82, 0x81, 0x82, 0 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED4, arraysize(EXPECTED4), data, length);

  // Check nothing changed on the other output.
  OLA_ASSERT_EQ(reinterpret_cast<const uint8_t*>(NULL),
                backend.GetData(1, &length));
  OLA_ASSERT_EQ(0u, backend.Writes(1));
}

/**
 * Test DMX writes in the individual P9813 mode.
 */
void SPIOutputTest::testIndividualP9813Control() {
  FakeSPIBackend backend(2);
  SPIOutput::Options options(0, "Test SPI Device");
  options.pixel_count = 2;
  SPIOutput output(m_uid, &backend, options);
  output.SetPersonality(5);

  DmxBuffer buffer;
  unsigned int length = 0;
  const uint8_t *data = NULL;

  buffer.SetFromString("1, 10, 100");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED0[] = { 0, 0, 0, 0, 0xef, 0x64, 0x0a, 0x01,
                                0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED0, arraysize(EXPECTED0), data, length);
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  buffer.SetFromString("255,128,0,10,20,30");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED1[] = { 0, 0, 0, 0, 0xf4, 0, 0x80, 0xff,
                                0xff, 0x1e, 0x14, 0x0a, 0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 0, 0, 0, 0, 0xef, 0x4e, 0x38, 0x22,
                                0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));

  buffer.SetFromString("7, 9");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));

  output.SetStartAddress(3);
  buffer.SetFromString("1,2,3,4,5,6,7,8");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED4[] = { 0, 0, 0, 0, 0xff, 0x05, 0x04, 0x03,
                                0xff, 0x08, 0x07, 0x06, 0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED4, arraysize(EXPECTED4), data, length);
  OLA_ASSERT_EQ(4u, backend.Writes(0));

  // Check nothing changed on the other output.
  OLA_ASSERT_EQ(reinterpret_cast<const uint8_t*>(NULL),
                backend.GetData(1, &length));
  OLA_ASSERT_EQ(0u, backend.Writes(1));
}

/**
 * Test DMX writes in the combined P9813 mode.
 */
void SPIOutputTest::testCombinedP9813Control() {
  FakeSPIBackend backend(2);
  SPIOutput::Options options(0, "Test SPI Device");
  options.pixel_count = 2;
  SPIOutput output(m_uid, &backend, options);
  output.SetPersonality(6);

  DmxBuffer buffer;
  buffer.SetFromString("255,128,0,10,20,30");
  output.WriteDMX(buffer);

  unsigned int length = 0;
  const uint8_t *data = backend.GetData(0, &length);

  const uint8_t EXPECTED1[] = { 0, 0, 0, 0, 0xf4, 0, 0x80, 0xff,
                                0xf4, 0, 0x80, 0xff, 0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 0, 0, 0, 0, 0xef, 0x4e, 0x38, 0x22,
                                0xef, 0x4e, 0x38, 0x22, 0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  buffer.SetFromString("7, 9");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  output.SetStartAddress(3);
  buffer.SetFromString("1,2,3,4,5,6,7,8");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED4[] = { 0, 0, 0, 0, 0xff, 0x05, 0x04, 0x03,
                                0xff, 0x05, 0x04, 0x03, 0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED4, arraysize(EXPECTED4), data, length);

  // Check nothing changed on the other output.
  OLA_ASSERT_EQ(reinterpret_cast<const uint8_t*>(NULL),
                backend.GetData(1, &length));
  OLA_ASSERT_EQ(0u, backend.Writes(1));
}

/**
 * Test DMX writes in the individual APA102 mode.
 */
void SPIOutputTest::testIndividualAPA102Control() {
  // setup Backend
  FakeSPIBackend backend(2);
  SPIOutput::Options options(0, "Test SPI Device");
  // setup pixel_count to 2 (enough to test all cases)
  options.pixel_count = 2;
  // setup SPIOutput
  SPIOutput output(m_uid, &backend, options);
  // set personality to 7= Individual APA102
  output.SetPersonality(7);
  
  // simulate incoming dmx data with this buffer
  DmxBuffer buffer;
  // setup an pointer to the returned data (the fake SPI data stream)
  unsigned int length = 0;
  const uint8_t *data = NULL;

  // test1
  // setup some 'DMX' data
  buffer.SetFromString("1, 10, 100");
  // simulate incoming data
  output.WriteDMX(buffer);
  // get fake SPI data stream
  data = backend.GetData(0, &length);
  // this is the expected spi data stream:
  const uint8_t EXPECTED0[] = { 0, 0, 0, 0,               // StartFrame
                                0xff, 0x64, 0x0a, 0x01,   // first Pixel
                                0xff, 0, 0, 0,            // second Pixel
                                0, 0, 0, 0, 0, 0, 0, 0};  // EndFrame
  // check for Equality
  OLA_ASSERT_DATA_EQUALS(EXPECTED0, arraysize(EXPECTED0), data, length);
  // check if the output writes are 1
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  // test2
  buffer.SetFromString("255,128,0,10,20,30");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED1[] = { 0, 0, 0, 0, 
                                0xff, 0, 0x80, 0xff,
                                0xff, 0x1e, 0x14, 0x0a, 
                                0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));
  
  // test3
  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 0, 0, 0, 0, 
                                0xff, 0x4e, 0x38, 0x22,
                                0xff, 0, 0, 0, 
                                0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));
  
  // test4
  // tests what happens if fewer then needed color information are received
  buffer.SetFromString("7, 9");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  // check that the returns are the same as test2 (nothing changed)
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));

  // test5
  // test with other StartAddress
  // set StartAddress
  output.SetStartAddress(3);
  // values 1 & 2 should not be visibel in SPI data stream
  buffer.SetFromString("1,2,3,4,5,6,7,8");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED4[] = { 0, 0, 0, 0, 
                                0xff, 0x05, 0x04, 0x03,
                                0xff, 0x08, 0x07, 0x06, 
                                0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED4, arraysize(EXPECTED4), data, length);
  OLA_ASSERT_EQ(4u, backend.Writes(0));
  
  // test6
  // Check nothing changed on the other output.
  OLA_ASSERT_EQ(reinterpret_cast<const uint8_t*>(NULL),
                backend.GetData(1, &length));
  OLA_ASSERT_EQ(0u, backend.Writes(1));
}

/**
 * Test DMX writes in the combined APA102 mode.
 */
void SPIOutputTest::testCombinedAPA102Control() {
  // setup Backend
  FakeSPIBackend backend(2);
  SPIOutput::Options options(0, "Test SPI Device");
  // setup pixel_count to 2 (enough to test all cases)
  options.pixel_count = 2;
  // setup SPIOutput
  SPIOutput output(m_uid, &backend, options);
  // set personality to 7= Individual APA102
  output.SetPersonality(8);
  
  // simulate incoming dmx data with this buffer
  DmxBuffer buffer;
  // setup an pointer to the returned data (the fake SPI data stream)
  unsigned int length = 0;
  const uint8_t *data = NULL;
  
  OLA_INFO << "Test Not implemented yet.";
  OLA_INFO << "length" << length;
  OLA_INFO << "data" << data;
  // TODO
  
}