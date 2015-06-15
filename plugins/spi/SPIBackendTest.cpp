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
 * SPIBackendTest.cpp
 * Test fixture for the SPIBackendTests.
 * Copyright (C) 2013 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "ola/base/Array.h"
#include "ola/DmxBuffer.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "plugins/spi/FakeSPIWriter.h"
#include "plugins/spi/SPIBackend.h"

using ola::DmxBuffer;
using ola::ExportMap;
using ola::plugin::spi::FakeSPIWriter;
using ola::plugin::spi::HardwareBackend;
using ola::plugin::spi::SoftwareBackend;
using ola::plugin::spi::SPIBackendInterface;
using ola::UIntMap;

class SPIBackendTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SPIBackendTest);
  CPPUNIT_TEST(testHardwareDrops);
  CPPUNIT_TEST(testHardwareVariousFrameLengths);
  CPPUNIT_TEST(testInvalidOutputs);
  CPPUNIT_TEST(testSoftwareDrops);
  CPPUNIT_TEST(testSoftwareVariousFrameLengths);
  CPPUNIT_TEST_SUITE_END();

 public:
  SPIBackendTest();

  void setUp();
  unsigned int DropCount();
  bool SendSomeData(SPIBackendInterface *backend,
                    uint8_t output,
                    const uint8_t *data,
                    unsigned int length,
                    unsigned int checkout_size,
                    unsigned int latch_bytes = 0);

  void testHardwareDrops();
  void testHardwareVariousFrameLengths();
  void testInvalidOutputs();
  void testSoftwareDrops();
  void testSoftwareVariousFrameLengths();

 private:
  ExportMap m_export_map;
  FakeSPIWriter m_writer;
  unsigned int m_total_size;

  static const uint8_t DATA1[];
  static const uint8_t DATA2[];
  static const uint8_t DATA3[];
  static const uint8_t EXPECTED1[];
  static const uint8_t EXPECTED2[];
  static const uint8_t EXPECTED3[];
  static const uint8_t EXPECTED4[];
  static const char DEVICE_NAME[];
  static const char SPI_DROP_VAR[];
  static const char SPI_DROP_VAR_KEY[];
};

const uint8_t SPIBackendTest::DATA1[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 0
};

const uint8_t SPIBackendTest::DATA2[] = {
  0xa, 0xb, 0xc, 0xd, 0xe, 0xf
};

const uint8_t SPIBackendTest::DATA3[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
  0xa, 0xb, 0xc, 0xd, 0xe, 0xf
};

const uint8_t SPIBackendTest::EXPECTED1[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0
};

const uint8_t SPIBackendTest::EXPECTED2[] = {
  0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0
};

const uint8_t SPIBackendTest::EXPECTED3[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
  0, 0, 0, 0,
};

const uint8_t SPIBackendTest::EXPECTED4[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0
};

const char SPIBackendTest::DEVICE_NAME[] = "Fake Device";
const char SPIBackendTest::SPI_DROP_VAR[] = "spi-drops";
const char SPIBackendTest::SPI_DROP_VAR_KEY[] = "device";


CPPUNIT_TEST_SUITE_REGISTRATION(SPIBackendTest);

SPIBackendTest::SPIBackendTest()
    : CppUnit::TestFixture(),
      m_writer(DEVICE_NAME) {
  m_total_size = arraysize(DATA3);
}

void SPIBackendTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
}

unsigned int SPIBackendTest::DropCount() {
  UIntMap *drop_map = m_export_map.GetUIntMapVar(SPI_DROP_VAR,
                                                 SPI_DROP_VAR_KEY);
  return (*drop_map)[DEVICE_NAME];
}

bool SPIBackendTest::SendSomeData(SPIBackendInterface *backend,
                                  uint8_t output,
                                  const uint8_t *data,
                                  unsigned int length,
                                  unsigned int checkout_size,
                                  unsigned int latch_bytes) {
  uint8_t *buffer = backend->Checkout(output, checkout_size, latch_bytes);
  if (!buffer)
    return false;
  memcpy(buffer, data, length);
  backend->Commit(output);
  return true;
}

/**
 * Check that we increment the exported variable when we drop frames.
 */
void SPIBackendTest::testHardwareDrops() {
  HardwareBackend backend(HardwareBackend::Options(), &m_writer,
                          &m_export_map);
  OLA_ASSERT(backend.Init());

  m_writer.BlockWriter();
  OLA_ASSERT_EQ(0u, DropCount());

  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  m_writer.WaitForWrite();  // now we know the writer is blocked
  OLA_ASSERT_EQ(1u, m_writer.WriteCount());
  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  OLA_ASSERT_EQ(1u, DropCount());
  m_writer.ResetWrite();
  m_writer.UnblockWriter();
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(2u, m_writer.WriteCount());
}

/**
 * Check that we handle the case of frame lengths changing.
 */
void SPIBackendTest::testHardwareVariousFrameLengths() {
  HardwareBackend backend(HardwareBackend::Options(), &m_writer,
                          &m_export_map);
  OLA_ASSERT(backend.Init());

  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(1u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED1, arraysize(EXPECTED1));
  m_writer.ResetWrite();

  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(2u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED1, arraysize(EXPECTED1));
  m_writer.ResetWrite();

  OLA_ASSERT(SendSomeData(&backend, 0, DATA2, arraysize(DATA2), m_total_size));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(3u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED2, arraysize(EXPECTED2));
  m_writer.ResetWrite();

  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(4u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED1, arraysize(EXPECTED1));
  m_writer.ResetWrite();

  OLA_ASSERT(SendSomeData(&backend, 0, DATA3, arraysize(DATA3), m_total_size));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(5u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), DATA3, arraysize(DATA3));
  m_writer.ResetWrite();

  // now test the latch bytes
  OLA_ASSERT(
      SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size, 4));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(6u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED3, arraysize(EXPECTED3));
  m_writer.ResetWrite();

  OLA_ASSERT(
      SendSomeData(&backend, 0, DATA3, arraysize(DATA3), m_total_size, 4));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(7u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED3, arraysize(EXPECTED3));
  m_writer.ResetWrite();
}

/**
 * Check we can't send to invalid outputs.
 */
void SPIBackendTest::testInvalidOutputs() {
  // HardwareBackend
  HardwareBackend hw_backend(HardwareBackend::Options(), &m_writer,
                             &m_export_map);
  OLA_ASSERT(hw_backend.Init());

  OLA_ASSERT_FALSE(
      SendSomeData(&hw_backend, 1, DATA1, arraysize(DATA1), m_total_size));
  OLA_ASSERT_EQ(0u, m_writer.WriteCount());

  // SoftwareBackend
  SoftwareBackend sw_backend(SoftwareBackend::Options(), &m_writer,
                             &m_export_map);
  OLA_ASSERT(sw_backend.Init());
  OLA_ASSERT_FALSE(
      SendSomeData(&sw_backend, 1, DATA1, arraysize(DATA1), m_total_size));
  OLA_ASSERT_EQ(0u, m_writer.WriteCount());
}

/**
 * Check that we increment the exported variable when we drop frames.
 */
void SPIBackendTest::testSoftwareDrops() {
  SoftwareBackend backend(SoftwareBackend::Options(), &m_writer,
                          &m_export_map);
  OLA_ASSERT(backend.Init());

  m_writer.BlockWriter();
  OLA_ASSERT_EQ(0u, DropCount());

  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  m_writer.WaitForWrite();  // now we know the writer is blocked
  OLA_ASSERT_EQ(1u, m_writer.WriteCount());
  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  OLA_ASSERT_EQ(1u, DropCount());
  m_writer.ResetWrite();
  m_writer.UnblockWriter();
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(2u, m_writer.WriteCount());
}

/**
 * Check that we handle the case of frame lengths changing.
 */
void SPIBackendTest::testSoftwareVariousFrameLengths() {
  SoftwareBackend backend(SoftwareBackend::Options(), &m_writer,
                          &m_export_map);
  OLA_ASSERT(backend.Init());

  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(1u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED1, arraysize(EXPECTED1));
  m_writer.ResetWrite();

  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(2u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED1, arraysize(EXPECTED1));
  m_writer.ResetWrite();

  OLA_ASSERT(SendSomeData(&backend, 0, DATA2, arraysize(DATA2), m_total_size));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(3u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED2, arraysize(EXPECTED2));
  m_writer.ResetWrite();

  OLA_ASSERT(SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(4u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED1, arraysize(EXPECTED1));
  m_writer.ResetWrite();

  OLA_ASSERT(SendSomeData(&backend, 0, DATA3, arraysize(DATA3), m_total_size));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(5u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), DATA3, arraysize(DATA3));
  m_writer.ResetWrite();

  // now test the latch bytes
  OLA_ASSERT(
      SendSomeData(&backend, 0, DATA1, arraysize(DATA1), m_total_size, 4));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(6u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED4, arraysize(EXPECTED4));
  m_writer.ResetWrite();

  OLA_ASSERT(
      SendSomeData(&backend, 0, DATA3, arraysize(DATA3), m_total_size, 4));
  m_writer.WaitForWrite();
  OLA_ASSERT_EQ(7u, m_writer.WriteCount());
  m_writer.CheckDataMatches(OLA_SOURCELINE(), EXPECTED3, arraysize(EXPECTED3));
  m_writer.ResetWrite();
}
