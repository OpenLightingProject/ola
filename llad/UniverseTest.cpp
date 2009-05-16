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

#include <string>
#include <cppunit/extensions/HelperMacros.h>

#include <lla/DmxBuffer.h>
#include <llad/Universe.h>
#include <llad/Preferences.h>
#include <llad/Port.h>
#include "Client.h"
#include "UniverseStore.h"

using namespace lla;
using namespace std;

static unsigned int TEST_UNIVERSE = 1;
static const string TEST_DATA = "this is some test data";

class UniverseTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UniverseTest);
  CPPUNIT_TEST(testLifecycle);
  CPPUNIT_TEST(testSetGet);
  CPPUNIT_TEST(testSendDmx);
  CPPUNIT_TEST(testReceiveDmx);
  CPPUNIT_TEST(testAddRemoveClients);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testLifecycle();
    void testSetGet();
    void testSendDmx();
    void testReceiveDmx();
    void testAddRemoveClients();

  private:
    MemoryPreferences *m_preferences;
    UniverseStore *m_store;
    DmxBuffer m_buffer;
};


class MockPort: public Port {
  public:
    MockPort(AbstractDevice *parent, unsigned int port_id):
      Port(parent, port_id) {}
    ~MockPort() {}

    bool WriteDMX(const DmxBuffer &buffer) { m_buffer = buffer; }
    const DmxBuffer &ReadDMX() const { return m_buffer; }
  private:
    DmxBuffer m_buffer;
};


class MockClient: public Client {
  public:
    MockClient(): Client(NULL) {}
    bool SendDMX(unsigned int universe_id, const DmxBuffer &buffer) {
      CPPUNIT_ASSERT_EQUAL(TEST_UNIVERSE, universe_id);
      CPPUNIT_ASSERT_EQUAL(TEST_DATA, buffer.Get());
      return true;
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(UniverseTest);


void UniverseTest::setUp() {
  m_preferences = new MemoryPreferences("foo");
  m_store = new UniverseStore(m_preferences, NULL);
  m_buffer.Set(TEST_DATA);
}


void UniverseTest::tearDown() {
  m_store->DeleteAll();
  delete m_preferences;
  delete m_store;
}


/*
 * Test that we can create universes and save their settings
 */
void UniverseTest::testLifecycle() {
  Universe *universe = m_store->GetUniverse(TEST_UNIVERSE);
  CPPUNIT_ASSERT(!universe);

  universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);
  CPPUNIT_ASSERT_EQUAL(universe->UniverseId(), TEST_UNIVERSE);
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), 1);
  CPPUNIT_ASSERT(!universe->IsActive());

  string universe_name = "New Name";
  universe->SetName(universe_name);
  universe->SetMergeMode(Universe::MERGE_HTP);

  CPPUNIT_ASSERT_EQUAL(universe->Name(), universe_name);
  CPPUNIT_ASSERT_EQUAL(universe->MergeMode(), Universe::MERGE_HTP);

  // delete it
  m_store->AddUniverseGarbageCollection(universe);
  m_store->GarbageCollectUniverses();
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), 0);
  universe = m_store->GetUniverse(TEST_UNIVERSE);
  CPPUNIT_ASSERT(!universe);

  // now re-create it
  universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), 1);
  CPPUNIT_ASSERT_EQUAL(universe->UniverseId(), TEST_UNIVERSE);
  CPPUNIT_ASSERT_EQUAL(universe->Name(), universe_name);
  CPPUNIT_ASSERT_EQUAL(universe->MergeMode(), Universe::MERGE_HTP);

  m_store->DeleteAll();
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), 0);
}


/*
 * Check that SetDMX/GetDMX works
 */
void UniverseTest::testSetGet() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);

  // a new universe should be all 0s
  DmxBuffer empty_buffer;
  CPPUNIT_ASSERT(empty_buffer == universe->GetDMX());

  // check that SetDMX works
  CPPUNIT_ASSERT(universe->SetDMX(m_buffer));
  CPPUNIT_ASSERT(m_buffer == universe->GetDMX());
}


/*
 * Check that SendDmx updates all ports
 */
void UniverseTest::testSendDmx() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);

  MockPort port(NULL, 1);
  universe->AddPort(&port);
  CPPUNIT_ASSERT_EQUAL(universe->PortCount(), 1);
  CPPUNIT_ASSERT(universe->IsActive());

  // send some data to the universe and check the port gets it
  CPPUNIT_ASSERT(universe->SetDMX(m_buffer));
  CPPUNIT_ASSERT(m_buffer == port.ReadDMX());

  // remove the port from the universe
  universe->RemovePort(&port);
  CPPUNIT_ASSERT_EQUAL(universe->PortCount(), 0);
  CPPUNIT_ASSERT(!universe->IsActive());
}


/*
 * Check that we update when ports have new data
 */
void UniverseTest::testReceiveDmx() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);

  MockPort port(NULL, 1);
  universe->AddPort(&port);
  CPPUNIT_ASSERT_EQUAL(universe->PortCount(), 1);
  CPPUNIT_ASSERT(universe->IsActive());

  // Setup the port with some data, and check that signalling the universe
  // works.
  port.WriteDMX(m_buffer);
  universe->PortDataChanged(&port);
  CPPUNIT_ASSERT(m_buffer == universe->GetDMX());

  // Remove the port from the universe
  universe->RemovePort(&port);
  CPPUNIT_ASSERT(!universe->IsActive());
  CPPUNIT_ASSERT_EQUAL(universe->PortCount(), 0);
}


/*
 * Check that we can add/remove clients from this universes
 */
void UniverseTest::testAddRemoveClients() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->ClientCount());

  // test that we can add a client
  MockClient client;
  universe->AddClient(&client);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, universe->ClientCount());
  CPPUNIT_ASSERT(universe->ContainsClient(&client));

  // Now set some data
  CPPUNIT_ASSERT(universe->SetDMX(m_buffer));

  // now remove it
  universe->RemoveClient(&client);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->ClientCount());
  CPPUNIT_ASSERT(!universe->ContainsClient(&client));
}
