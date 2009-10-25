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
 * DMPAddressTest.cpp
 * Test fixture for the DMPAddress class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include <ola/network/NetworkUtils.h>
#include "PDUTestCommon.h"
#include "DMPAddress.h"

namespace ola {
namespace e131 {

using ola::network::NetworkToHost;

class DMPAddressTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DMPAddressTest);
  CPPUNIT_TEST(testAddress);
  CPPUNIT_TEST(testRangeAddress);
  CPPUNIT_TEST(testAddressData);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testAddress();
    void testRangeAddress();
    void testAddressData();
  private:
    void checkAddress(
        const BaseDMPAddress *address,
        unsigned int start,
        unsigned int increment,
        unsigned int number,
        unsigned int size,
        dmp_address_size address_size,
        bool is_range);
};

CPPUNIT_TEST_SUITE_REGISTRATION(DMPAddressTest);


/*
 * Check the property match, pack the address, decode it, and check the decoded
 * address matches the first one.
 */
void DMPAddressTest::checkAddress(
    const BaseDMPAddress *address,
    unsigned int start,
    unsigned int increment,
    unsigned int number,
    unsigned int size,
    dmp_address_size address_size,
    bool is_range) {

  CPPUNIT_ASSERT_EQUAL(size, address->Size());
  CPPUNIT_ASSERT_EQUAL(address_size, address->AddressSize());
  CPPUNIT_ASSERT_EQUAL(is_range, address->IsRange());
  CPPUNIT_ASSERT_EQUAL(start, address->Start());
  CPPUNIT_ASSERT_EQUAL(increment, address->Increment());
  CPPUNIT_ASSERT_EQUAL(number, address->Number());

  unsigned int length = address->Size();
  uint8_t *buffer = new uint8_t[length];
  CPPUNIT_ASSERT(address->Pack(buffer, length));
  CPPUNIT_ASSERT_EQUAL(size, length);

  const BaseDMPAddress *addr = DecodeAddress(
      address_size,
      is_range ? RANGE_SINGLE: NON_RANGE,
      buffer, length);
  CPPUNIT_ASSERT_EQUAL(size, length);
  CPPUNIT_ASSERT_EQUAL(start, address->Start());
  CPPUNIT_ASSERT_EQUAL(increment, address->Increment());
  CPPUNIT_ASSERT_EQUAL(number, address->Number());

  delete[] buffer;
  delete addr;
}


/*
 * Test that addresses work.
 */
void DMPAddressTest::testAddress() {
  uint32_t temp = 0;
  unsigned int length = sizeof(temp);

  OneByteDMPAddress addr1(10);
  checkAddress(&addr1, 10, 0, 1, 1, ONE_BYTES, false);
  TwoByteDMPAddress addr2(1024);
  checkAddress(&addr2, 1024, 0, 1, 2, TWO_BYTES, false);
  FourByteDMPAddress addr3(66000);
  checkAddress(&addr3, 66000, 0, 1, 4, FOUR_BYTES, false);

  const BaseDMPAddress *addr4 = NewSingleAddress(10);
  checkAddress(addr4, 10, 0, 1, 1, ONE_BYTES, false);
  delete addr4;

  const BaseDMPAddress *addr5 = NewSingleAddress(1024);
  checkAddress(addr5, 1024, 0, 1, 2, TWO_BYTES, false);
  delete addr5;

  const BaseDMPAddress *addr6 = NewSingleAddress(66000);
  checkAddress(addr6, 66000, 0, 1, 4, FOUR_BYTES, false);
  delete addr6;
}


/*
 * Test that Ranged Addresses work
 */
void DMPAddressTest::testRangeAddress() {
  uint8_t buffer[12];
  uint16_t *p = (uint16_t*) buffer;
  uint32_t *pp = (uint32_t*) buffer;
  unsigned int length = sizeof(buffer);

  OneByteRangeDMPAddress addr1(10, 2, 4);
  checkAddress(&addr1, 10, 2, 4, 3, ONE_BYTES, true);
  CPPUNIT_ASSERT(addr1.Pack(buffer, length));
  CPPUNIT_ASSERT_EQUAL(addr1.Size(), length);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 10, buffer[0]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 2, buffer[1]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 4, buffer[2]);

  length = sizeof(buffer);
  TwoByteRangeDMPAddress addr2(1024, 2, 99);
  checkAddress(&addr2, 1024, 2, 99, 6, TWO_BYTES, true);
  CPPUNIT_ASSERT(addr2.Pack(buffer, length));
  CPPUNIT_ASSERT_EQUAL(addr2.Size(), length);
  CPPUNIT_ASSERT_EQUAL((uint16_t) 1024, NetworkToHost(*p++));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 2, NetworkToHost(*p++));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 99, NetworkToHost(*p));

  length = sizeof(buffer);
  FourByteRangeDMPAddress addr3(66000, 2, 100);
  checkAddress(&addr3, 66000, 2, 100, 12, FOUR_BYTES, true);
  CPPUNIT_ASSERT(addr3.Pack(buffer, length));
  CPPUNIT_ASSERT_EQUAL(addr3.Size(), length);
  CPPUNIT_ASSERT_EQUAL((uint32_t) 66000, NetworkToHost(*pp++));
  CPPUNIT_ASSERT_EQUAL((uint32_t) 2, NetworkToHost(*pp++));
  CPPUNIT_ASSERT_EQUAL((uint32_t) 100, NetworkToHost(*pp));

  const BaseDMPAddress *addr4 = NewRangeAddress(10, 1, 10);
  length = sizeof(buffer);
  checkAddress(addr4, 10, 1, 10, 3, ONE_BYTES, true);
  CPPUNIT_ASSERT(addr4->Pack(buffer, length));
  CPPUNIT_ASSERT_EQUAL(addr4->Size(), length);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 10, buffer[0]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, buffer[1]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 10, buffer[2]);
  delete addr4;

  p = (uint16_t*) buffer;
  const BaseDMPAddress *addr5 = NewRangeAddress(10, 1, 1024);
  length = sizeof(buffer);
  checkAddress(addr5, 10, 1, 1024, 6, TWO_BYTES, true);
  CPPUNIT_ASSERT(addr5->Pack(buffer, length));
  CPPUNIT_ASSERT_EQUAL(addr5->Size(), length);
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, NetworkToHost(*p++));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 1, NetworkToHost(*p++));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 1024, NetworkToHost(*p));
  delete addr5;

  pp = (uint32_t*) buffer;
  const BaseDMPAddress *addr6 = NewRangeAddress(66000, 1, 1024);
  length = sizeof(buffer);
  checkAddress(addr6, 66000, 1, 1024, 12, FOUR_BYTES, true);
  CPPUNIT_ASSERT(addr6->Pack(buffer, length));
  CPPUNIT_ASSERT_EQUAL(addr6->Size(), length);
  CPPUNIT_ASSERT_EQUAL((uint32_t) 66000, NetworkToHost(*pp++));
  CPPUNIT_ASSERT_EQUAL((uint32_t) 1, NetworkToHost(*pp++));
  CPPUNIT_ASSERT_EQUAL((uint32_t) 1024, NetworkToHost(*pp));
  delete addr6;

}


/*
 * test that AddressData objects work
 */
void DMPAddressTest::testAddressData() {
  uint8_t buffer[12];
  unsigned int length = sizeof(buffer);
  OneByteDMPAddress addr1(10);
  DMPAddressData<OneByteDMPAddress> chunk(&addr1, NULL, 0);

  CPPUNIT_ASSERT_EQUAL((const OneByteDMPAddress*) &addr1, chunk.Address());
  CPPUNIT_ASSERT_EQUAL((const uint8_t*) NULL, chunk.Data());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, chunk.Size());
  CPPUNIT_ASSERT(!chunk.Pack(buffer, length));

  length = sizeof(buffer);
  TwoByteRangeDMPAddress addr2(10, 2, 10);
  DMPAddressData<TwoByteRangeDMPAddress> chunk2(&addr2, NULL, 0);

  CPPUNIT_ASSERT_EQUAL((const TwoByteRangeDMPAddress*) &addr2,
                       chunk2.Address());
  CPPUNIT_ASSERT_EQUAL((const uint8_t*) NULL, chunk2.Data());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 6, chunk2.Size());
  CPPUNIT_ASSERT(!chunk2.Pack(buffer, length));
}


} // e131
} // ola
