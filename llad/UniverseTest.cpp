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
 * UniverseTest.cpp
 * Test fixture for the Universe and UniverseStore classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdint.h>
#include <string>
#include <cppunit/extensions/HelperMacros.h>

#include <llad/Universe.h>
#include <llad/Preferences.h>
#include <llad/Port.h>
#include "UniverseStore.h"

using namespace lla;
using namespace std;

static char TEST_DMX_DATA[] = "this is some test data";


class UniverseTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UniverseTest);
  CPPUNIT_TEST(testLifecycle);
  CPPUNIT_TEST(testSendDmx);
  CPPUNIT_TEST(testReceiveDmx);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testLifecycle();
    void testSendDmx();
    void testReceiveDmx();

  private:
    MemoryPreferences *m_preferences;
    UniverseStore *m_store;
};


class MockPort: public Port {
  public:
    MockPort(AbstractDevice *parent, int port_id);
    ~MockPort() { free(m_data); }

    int WriteDMX(uint8_t *data, unsigned int length);
    int ReadDMX(uint8_t *data, unsigned int length);
  private:
    uint8_t *m_data;
    unsigned int m_length;
    static const int PORT_BUFFER_SIZE = 40;
};

CPPUNIT_TEST_SUITE_REGISTRATION(UniverseTest);


MockPort::MockPort(AbstractDevice *parent, int port_id): Port(parent, port_id) {
  m_data = (uint8_t*) malloc(PORT_BUFFER_SIZE);
  CPPUNIT_ASSERT(m_data);
}


int MockPort::ReadDMX(uint8_t *data, unsigned int length) {
  int l = length > m_length ? m_length : length;
  memcpy(data, m_data, l);
  return l;
}


int MockPort::WriteDMX(uint8_t *data, unsigned int length) {
  CPPUNIT_ASSERT(length < PORT_BUFFER_SIZE);
  memcpy(m_data, data, length);
  m_length = length;
}

void UniverseTest::setUp() {
  m_preferences = new MemoryPreferences("foo");
  m_store = new UniverseStore(m_preferences, NULL);
}


void UniverseTest::tearDown() {
  delete m_preferences;
  delete m_store;
}


/*
 * Test that we can create universes and save their settings
 */
void UniverseTest::testLifecycle() {

  int universe_id = 1;
  Universe *universe = m_store->GetUniverse(universe_id);
  CPPUNIT_ASSERT(!universe);

  universe = m_store->GetUniverseOrCreate(universe_id);
  CPPUNIT_ASSERT(universe);
  CPPUNIT_ASSERT_EQUAL(universe->UniverseId(), universe_id);
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), 1);
  CPPUNIT_ASSERT(!universe->IsActive());

  string universe_name = "New Name";
  universe->SetName(universe_name);
  universe->SetMergeMode(Universe::MERGE_HTP);

  CPPUNIT_ASSERT_EQUAL(universe->Name(), universe_name);
  CPPUNIT_ASSERT_EQUAL(universe->MergeMode(), Universe::MERGE_HTP);

  // delete it
  CPPUNIT_ASSERT(m_store->DeleteUniverseIfInactive(universe));
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), 0);
  universe = m_store->GetUniverse(universe_id);
  CPPUNIT_ASSERT(!universe);

  // now re-create it
  universe = m_store->GetUniverseOrCreate(universe_id);
  CPPUNIT_ASSERT(universe);
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), 1);
  CPPUNIT_ASSERT_EQUAL(universe->UniverseId(), universe_id);
  CPPUNIT_ASSERT_EQUAL(universe->Name(), universe_name);
  CPPUNIT_ASSERT_EQUAL(universe->MergeMode(), Universe::MERGE_HTP);

  m_store->DeleteAll();
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), 0);
}


void UniverseTest::testSendDmx() {
  int universe_id = 1;
  Universe *universe = m_store->GetUniverseOrCreate(universe_id);
  CPPUNIT_ASSERT(universe);

  MockPort port(NULL, 1);
  universe->AddPort(&port);
  CPPUNIT_ASSERT_EQUAL(universe->PortCount(), 1);
  CPPUNIT_ASSERT(universe->IsActive());

  // send some data to the universe and check the port gets it
  unsigned int data_size = sizeof(TEST_DMX_DATA);
  universe->SetDMX((uint8_t*) TEST_DMX_DATA, data_size);
  uint8_t *data = (uint8_t*) malloc(data_size);
  CPPUNIT_ASSERT(data);
  int data_received = port.ReadDMX(data, data_size);
  CPPUNIT_ASSERT_EQUAL(data_received, (int) data_size);
  CPPUNIT_ASSERT(!memcmp(data, TEST_DMX_DATA, data_received));

  // remove the port from the universe
  universe->RemovePort(&port);
  CPPUNIT_ASSERT_EQUAL(universe->PortCount(), 0);
  CPPUNIT_ASSERT(!universe->IsActive());
  m_store->DeleteUniverseIfInactive(universe);
  free(data);
}


void UniverseTest::testReceiveDmx() {
  int universe_id = 1;
  Universe *universe = m_store->GetUniverseOrCreate(universe_id);
  CPPUNIT_ASSERT(universe);

  MockPort port(NULL, 1);
  universe->AddPort(&port);
  CPPUNIT_ASSERT_EQUAL(universe->PortCount(), 1);
  CPPUNIT_ASSERT(universe->IsActive());

  // setup the port with some data, and check that signalling the universe
  // works.
  unsigned int data_size = sizeof(TEST_DMX_DATA);
  port.WriteDMX((uint8_t*) TEST_DMX_DATA, data_size);
  universe->PortDataChanged(&port);

  uint8_t *data = (uint8_t*) malloc(data_size);
  CPPUNIT_ASSERT(data);
  int data_received = universe->GetDMX(data, data_size);
  CPPUNIT_ASSERT_EQUAL(data_received, (int) data_size);
  CPPUNIT_ASSERT(!memcmp(data, TEST_DMX_DATA, data_received));

  // remove the port from the universe
  universe->RemovePort(&port);
  CPPUNIT_ASSERT(!universe->IsActive());
  CPPUNIT_ASSERT_EQUAL(universe->PortCount(), 0);
  m_store->DeleteUniverseIfInactive(universe);
  free(data);
}
