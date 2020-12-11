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
 * Check the description, start address & personality.
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
  const uint8_t EXPECTED0[] = { 0x85, 0x80, 0xB2, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED0, arraysize(EXPECTED0), data, length);
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  buffer.SetFromString("255,128,0,10,20,30");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED1[] = { 0xC0, 0xFF, 0x80, 0x8A, 0x85, 0x8F, 0 };
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 0x9C, 0x91, 0xA7, 0x8A, 0x85, 0x8F, 0 };
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

  const uint8_t EXPECTED1[] = { 0xC0, 0xFF, 0x80, 0xC0, 0xFF, 0x80, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 0x9C, 0x91, 0xA7, 0x9C, 0x91, 0xA7, 0 };
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
  const uint8_t EXPECTED0[] = { 0, 0, 0, 0, 0xEF, 0x64, 0x0A, 0x01,
                                0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED0, arraysize(EXPECTED0), data, length);
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  buffer.SetFromString("255,128,0,10,20,30");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED1[] = { 0, 0, 0, 0, 0xF4, 0, 0x80, 0xFF,
                                0xFF, 0x1E, 0x14, 0x0A, 0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 0, 0, 0, 0, 0xEF, 0x4E, 0x38, 0x22,
                                0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
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
  const uint8_t EXPECTED4[] = { 0, 0, 0, 0, 0xFF, 0x05, 0x04, 0x03,
                                0xFF, 0x08, 0x07, 0x06, 0, 0, 0, 0, 0, 0, 0, 0};
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

  const uint8_t EXPECTED1[] = { 0, 0, 0, 0, 0xF4, 0, 0x80, 0xFF,
                                0xF4, 0, 0x80, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 0, 0, 0, 0, 0xEF, 0x4E, 0x38, 0x22,
                                0xEF, 0x4E, 0x38, 0x22, 0, 0, 0, 0, 0, 0, 0, 0};
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
  const uint8_t EXPECTED4[] = { 0, 0, 0, 0, 0xFF, 0x05, 0x04, 0x03,
                                0xFF, 0x05, 0x04, 0x03, 0, 0, 0, 0, 0, 0, 0, 0};
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
  // personality 7= Individual APA102
  const uint16_t this_test_personality = 7;
  // setup Backend
  FakeSPIBackend backend(2);
  SPIOutput::Options options(0, "Test SPI Device");
  // setup pixel_count to 2 (enough to test all cases)
  options.pixel_count = 2;
  // setup SPIOutput
  SPIOutput output(m_uid, &backend, options);
  // set personality
  output.SetPersonality(this_test_personality);

  // simulate incoming DMX data with this buffer
  DmxBuffer buffer;
  // setup a pointer to the returned data (the fake SPI data stream)
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
  const uint8_t EXPECTED1[] = { 0, 0, 0, 0,               // StartFrame
                                0xFF, 0x64, 0x0A, 0x01,   // first Pixel
                                0xFF, 0x00, 0x00, 0x00,   // second Pixel
                                0};                       // EndFrame
  // check for Equality
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  // check if the output writes are 1
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  // test2
  buffer.SetFromString("255,128,0,10,20,30");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 0, 0, 0, 0,
                                0xFF, 0x00, 0x80, 0xFF,
                                0xFF, 0x1E, 0x14, 0x0A,
                                0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  // test3
  // test what happens when only new data for the first leds is available.
  // later data should be not modified so for pixel2 data set in test2 is valid
  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED3[] = { 0, 0, 0, 0,
                                0xFF, 0x4E, 0x38, 0x22,
                                0xFF, 0x1E, 0x14, 0x0A,
                                0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED3, arraysize(EXPECTED3), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));

  // test4
  // tests what happens if fewer then needed color information are received
  buffer.SetFromString("7, 9");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  // check that the returns are the same as test3 (nothing changed)
  OLA_ASSERT_DATA_EQUALS(EXPECTED3, arraysize(EXPECTED3), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));

  // test5
  // test with changed StartAddress
  // set StartAddress
  output.SetStartAddress(3);
  // values 1 & 2 should not be visible in SPI data stream
  buffer.SetFromString("1,2,3,4,5,6,7,8");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED5[] = { 0, 0, 0, 0,
                                0xFF, 0x05, 0x04, 0x03,
                                0xFF, 0x08, 0x07, 0x06,
                                0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED5, arraysize(EXPECTED5), data, length);
  OLA_ASSERT_EQ(4u, backend.Writes(0));
  // change StartAddress back to default
  output.SetStartAddress(1);

  // test6
  // Check nothing changed on the other output.
  OLA_ASSERT_EQ(reinterpret_cast<const uint8_t*>(NULL),
                backend.GetData(1, &length));
  OLA_ASSERT_EQ(0u, backend.Writes(1));

  // test7
  // test for multiple ports
  // StartFrame is only allowed on first port.
  SPIOutput::Options options1(1, "second SPI Device");
  // setup pixel_count to 2 (enough to test all cases)
  options1.pixel_count = 2;
  // setup SPIOutput
  SPIOutput output1(m_uid, &backend, options1);
  // set personality
  output1.SetPersonality(this_test_personality);
  // setup some 'DMX' data
  buffer.SetFromString("1, 10, 100, 100, 10, 1");
  // simulate incoming data
  output1.WriteDMX(buffer);
  // get fake SPI data stream
  data = backend.GetData(1, &length);
  // this is the expected spi data stream:
  // StartFrame is missing --> port is >0 !
  const uint8_t EXPECTED7[] = { // 0, 0, 0, 0,            // StartFrame
                                0xFF, 0x64, 0x0A, 0x01,   // first Pixel
                                0xFF, 0x01, 0x0A, 0x64,   // second Pixel
                                0};                       // EndFrame
  // check for Equality
  OLA_ASSERT_DATA_EQUALS(EXPECTED7, arraysize(EXPECTED7), data, length);
  // check if the output writes are 1
  OLA_ASSERT_EQ(1u, backend.Writes(1));

  // test8
  // create new output with pixel_count=16 and check data length
  // setup pixel_count to 16
  options.pixel_count = 16;
  // setup SPIOutput
  SPIOutput output2(m_uid, &backend, options);
  // set personality
  output2.SetPersonality(this_test_personality);
  buffer.SetFromString(
        std::string("0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,") +
                    "0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,");
  output2.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED8[] = { 0, 0, 0, 0,
                                0xFF, 0, 0, 0,  // Pixel 1
                                0xFF, 0, 0, 0,  // Pixel 2
                                0xFF, 0, 0, 0,  // Pixel 3
                                0xFF, 0, 0, 0,  // Pixel 4
                                0xFF, 0, 0, 0,  // Pixel 5
                                0xFF, 0, 0, 0,  // Pixel 6
                                0xFF, 0, 0, 0,  // Pixel 7
                                0xFF, 0, 0, 0,  // Pixel 8
                                0xFF, 0, 0, 0,  // Pixel 9
                                0xFF, 0, 0, 0,  // Pixel 10
                                0xFF, 0, 0, 0,  // Pixel 11
                                0xFF, 0, 0, 0,  // Pixel 12
                                0xFF, 0, 0, 0,  // Pixel 13
                                0xFF, 0, 0, 0,  // Pixel 14
                                0xFF, 0, 0, 0,  // Pixel 15
                                0xFF, 0, 0, 0,  // Pixel 16
                                0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED8, arraysize(EXPECTED8), data, length);
  OLA_ASSERT_EQ(5u, backend.Writes(0));

  // test9
  // create new output with pixel_count=17 and check data length
  // setup pixel_count to 17
  options.pixel_count = 17;
  // setup SPIOutput
  SPIOutput output3(m_uid, &backend, options);
  // set personality
  output3.SetPersonality(this_test_personality);
  // generate dmx data
  buffer.SetFromString(
        std::string("0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,") +
                    "0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0," +
                    "0,0,0");
  output3.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED9[] = { 0, 0, 0, 0,
                                0xFF, 0, 0, 0,  // Pixel 1
                                0xFF, 0, 0, 0,  // Pixel 2
                                0xFF, 0, 0, 0,  // Pixel 3
                                0xFF, 0, 0, 0,  // Pixel 4
                                0xFF, 0, 0, 0,  // Pixel 5
                                0xFF, 0, 0, 0,  // Pixel 6
                                0xFF, 0, 0, 0,  // Pixel 7
                                0xFF, 0, 0, 0,  // Pixel 8
                                0xFF, 0, 0, 0,  // Pixel 9
                                0xFF, 0, 0, 0,  // Pixel 10
                                0xFF, 0, 0, 0,  // Pixel 11
                                0xFF, 0, 0, 0,  // Pixel 12
                                0xFF, 0, 0, 0,  // Pixel 13
                                0xFF, 0, 0, 0,  // Pixel 14
                                0xFF, 0, 0, 0,  // Pixel 15
                                0xFF, 0, 0, 0,  // Pixel 16
                                0xFF, 0, 0, 0,  // Pixel 17
                                0, 0};  // now we have two latch bytes...
  OLA_ASSERT_DATA_EQUALS(EXPECTED9, arraysize(EXPECTED9), data, length);
  OLA_ASSERT_EQ(6u, backend.Writes(0));
}

/**
 * Test DMX writes in the combined APA102 mode.
 */
void SPIOutputTest::testCombinedAPA102Control() {
  // personality 8= Combined APA102
  const uint16_t this_test_personality = 8;
  // setup Backend
  FakeSPIBackend backend(2);
  SPIOutput::Options options(0, "Test SPI Device");
  // setup pixel_count to 2 (enough to test all cases)
  options.pixel_count = 2;
  // setup SPIOutput
  SPIOutput output(m_uid, &backend, options);
  // set personality to 8= Combined APA102
  output.SetPersonality(this_test_personality);

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
  const uint8_t EXPECTED1[] = { 0, 0, 0, 0,               // StartFrame
                                0xFF, 0x64, 0x0A, 0x01,   // first Pixel
                                0xFF, 0x64, 0x0A, 0x01,   // second Pixel
                                0};                       // EndFrame
  // check for Equality
  OLA_ASSERT_DATA_EQUALS(EXPECTED1, arraysize(EXPECTED1), data, length);
  // check if the output writes are 1
  OLA_ASSERT_EQ(1u, backend.Writes(0));

  // test2
  buffer.SetFromString("255,128,0,10,20,30");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED2[] = { 0, 0, 0, 0,
                                0xFF, 0x00, 0x80, 0xFF,
                                0xFF, 0x00, 0x80, 0xFF,
                                0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED2, arraysize(EXPECTED2), data, length);
  OLA_ASSERT_EQ(2u, backend.Writes(0));

  // test3
  buffer.SetFromString("34,56,78");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED3[] = { 0, 0, 0, 0,
                                0xFF, 0x4E, 0x38, 0x22,
                                0xFF, 0x4E, 0x38, 0x22,
                                0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED3, arraysize(EXPECTED3), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));

  // test4
  // tests what happens if fewer then needed color information are received
  buffer.SetFromString("7, 9");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  // check that the returns are the same as test2 (nothing changed)
  OLA_ASSERT_DATA_EQUALS(EXPECTED3, arraysize(EXPECTED3), data, length);
  OLA_ASSERT_EQ(3u, backend.Writes(0));

  // test5
  // test with other StartAddress
  // set StartAddress
  output.SetStartAddress(3);
  // values 1 & 2 should not be visible in SPI data stream
  buffer.SetFromString("1,2,3,4,5,6,7,8");
  output.WriteDMX(buffer);
  data = backend.GetData(0, &length);
  const uint8_t EXPECTED5[] = { 0, 0, 0, 0,
                                0xFF, 0x05, 0x04, 0x03,
                                0xFF, 0x05, 0x04, 0x03,
                                0};
  OLA_ASSERT_DATA_EQUALS(EXPECTED5, arraysize(EXPECTED5), data, length);
  OLA_ASSERT_EQ(4u, backend.Writes(0));

  // test6
  // Check nothing changed on the other output.
  OLA_ASSERT_EQ(reinterpret_cast<const uint8_t*>(NULL),
                backend.GetData(1, &length));
  OLA_ASSERT_EQ(0u, backend.Writes(1));

  // test7
  // test for multiple ports
  // StartFrame is only allowed on first port.
  SPIOutput::Options option1(1, "second SPI Device");
  // setup pixel_count to 2 (enough to test all cases)
  option1.pixel_count = 2;
  // setup SPIOutput
  SPIOutput output1(m_uid, &backend, option1);
  // set personality
  output1.SetPersonality(this_test_personality);
  // setup some 'DMX' data
  buffer.SetFromString("1, 10, 100");
  // simulate incoming data
  output1.WriteDMX(buffer);
  // get fake SPI data stream
  data = backend.GetData(1, &length);
  // this is the expected spi data stream:
  // StartFrame is missing --> port is >0 !
  const uint8_t EXPECTED7[] = { // 0, 0, 0, 0,            // StartFrame
                                0xFF, 0x64, 0x0A, 0x01,   // first Pixel
                                0xFF, 0x64, 0x0A, 0x01,   // second Pixel
                                0};                       // EndFrame
  // check for Equality
  OLA_ASSERT_DATA_EQUALS(EXPECTED7, arraysize(EXPECTED7), data, length);
  // check if the output writes are 1
  OLA_ASSERT_EQ(1u, backend.Writes(1));
}
