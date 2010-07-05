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

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/BaseTypes.h"
#include "ola/DmxBuffer.h"
#include "ola/rdm/UID.h"
#include "olad/Client.h"
#include "olad/DmxSource.h"
#include "olad/Port.h"
#include "olad/PortManager.h"
#include "olad/Preferences.h"
#include "olad/TestCommon.h"
#include "olad/Universe.h"
#include "olad/UniverseStore.h"

using ola::AbstractDevice;
using ola::DmxBuffer;
using ola::Clock;
using ola::TimeStamp;
using ola::Universe;
using std::string;

static unsigned int TEST_UNIVERSE = 1;
static const char TEST_DATA[] = "this is some test data";


class UniverseTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UniverseTest);
  CPPUNIT_TEST(testLifecycle);
  CPPUNIT_TEST(testSetGet);
  CPPUNIT_TEST(testSendDmx);
  CPPUNIT_TEST(testReceiveDmx);
  CPPUNIT_TEST(testSourceClients);
  CPPUNIT_TEST(testSinkClients);
  CPPUNIT_TEST(testLtpMerging);
  CPPUNIT_TEST(testHtpMerging);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testLifecycle();
    void testSetGet();
    void testSendDmx();
    void testReceiveDmx();
    void testSourceClients();
    void testSinkClients();
    void testLtpMerging();
    void testHtpMerging();

  private:
    ola::MemoryPreferences *m_preferences;
    ola::UniverseStore *m_store;
    DmxBuffer m_buffer;
};


class MockClient: public ola::Client {
  public:
    MockClient(): ola::Client(NULL), m_dmx_set(false) {}
    bool SendDMX(unsigned int universe_id, const DmxBuffer &buffer) {
      CPPUNIT_ASSERT_EQUAL(TEST_UNIVERSE, universe_id);
      CPPUNIT_ASSERT_EQUAL(string(TEST_DATA), buffer.Get());
      m_dmx_set = true;
      return true;
    }
    bool m_dmx_set;
};


CPPUNIT_TEST_SUITE_REGISTRATION(UniverseTest);


void UniverseTest::setUp() {
  UID source_uid(OPEN_LIGHTING_ESTA_CODE, 0);
  m_preferences = new ola::MemoryPreferences("foo");
  m_store = new ola::UniverseStore(m_preferences, NULL, source_uid);
  m_buffer.Set(TEST_DATA);
}


void UniverseTest::tearDown() {
  delete m_store;
  delete m_preferences;
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
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), (unsigned int) 1);
  CPPUNIT_ASSERT_EQUAL(universe->MergeMode(), Universe::MERGE_LTP);
  CPPUNIT_ASSERT(!universe->IsActive());

  string universe_name = "New Name";
  universe->SetName(universe_name);
  universe->SetMergeMode(Universe::MERGE_HTP);

  CPPUNIT_ASSERT_EQUAL(universe->Name(), universe_name);
  CPPUNIT_ASSERT_EQUAL(universe->MergeMode(), Universe::MERGE_HTP);

  // delete it
  m_store->AddUniverseGarbageCollection(universe);
  m_store->GarbageCollectUniverses();
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), (unsigned int) 0);
  universe = m_store->GetUniverse(TEST_UNIVERSE);
  CPPUNIT_ASSERT(!universe);

  // now re-create it
  universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), (unsigned int) 1);
  CPPUNIT_ASSERT_EQUAL(universe->UniverseId(), TEST_UNIVERSE);
  CPPUNIT_ASSERT_EQUAL(universe->Name(), universe_name);
  CPPUNIT_ASSERT_EQUAL(universe->MergeMode(), Universe::MERGE_HTP);

  m_store->DeleteAll();
  CPPUNIT_ASSERT_EQUAL(m_store->UniverseCount(), (unsigned int) 0);
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

  TestMockOutputPort port(NULL, 1);  // output port
  universe->AddPort(&port);
  CPPUNIT_ASSERT_EQUAL(universe->InputPortCount(), (unsigned int) 0);
  CPPUNIT_ASSERT_EQUAL(universe->OutputPortCount(), (unsigned int) 1);
  CPPUNIT_ASSERT(universe->IsActive());

  // send some data to the universe and check the port gets it
  CPPUNIT_ASSERT(universe->SetDMX(m_buffer));
  CPPUNIT_ASSERT(m_buffer == port.ReadDMX());

  // remove the port from the universe
  universe->RemovePort(&port);
  CPPUNIT_ASSERT_EQUAL(universe->InputPortCount(), (unsigned int) 0);
  CPPUNIT_ASSERT_EQUAL(universe->OutputPortCount(), (unsigned int) 0);
  CPPUNIT_ASSERT(!universe->IsActive());
}


/*
 * Check that we update when ports have new data
 */
void UniverseTest::testReceiveDmx() {
  ola::PortManager port_manager(m_store);

  MockDevice device(NULL, "foo");
  TimeStamp time_stamp;
  TestMockInputPort port(&device, 1, &time_stamp);  // input port
  port_manager.PatchPort(&port, TEST_UNIVERSE);

  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);

  CPPUNIT_ASSERT_EQUAL(universe->InputPortCount(), (unsigned int) 1);
  CPPUNIT_ASSERT_EQUAL(universe->OutputPortCount(), (unsigned int) 0);
  CPPUNIT_ASSERT(universe->IsActive());

  // Setup the port with some data, and check that signalling the universe
  // works.
  Clock::CurrentTime(&time_stamp);
  port.WriteDMX(m_buffer);
  port.DmxChanged();
  CPPUNIT_ASSERT_EQUAL(ola::DmxSource::PRIORITY_DEFAULT,
                       universe->ActivePriority());
  CPPUNIT_ASSERT_EQUAL(m_buffer.Size(), universe->GetDMX().Size());
  CPPUNIT_ASSERT(m_buffer == universe->GetDMX());

  // Remove the port from the universe
  universe->RemovePort(&port);
  CPPUNIT_ASSERT(!universe->IsActive());
  CPPUNIT_ASSERT_EQUAL(universe->InputPortCount(), (unsigned int) 0);
  CPPUNIT_ASSERT_EQUAL(universe->OutputPortCount(), (unsigned int) 0);
}


/*
 * Check that we can add/remove source clients from this universes
 */
void UniverseTest::testSourceClients() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SourceClientCount());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SinkClientCount());

  // test that we can add a source client
  MockClient client;
  universe->AddSourceClient(&client);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, universe->SourceClientCount());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SinkClientCount());
  CPPUNIT_ASSERT(universe->ContainsSourceClient(&client));
  CPPUNIT_ASSERT(!universe->ContainsSinkClient(&client));
  CPPUNIT_ASSERT(universe->IsActive());

  // Setting DMX now does nothing
  CPPUNIT_ASSERT(!client.m_dmx_set);
  universe->SetDMX(m_buffer);
  CPPUNIT_ASSERT(!client.m_dmx_set);

  // now remove it
  universe->RemoveSourceClient(&client);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SourceClientCount());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SinkClientCount());
  CPPUNIT_ASSERT(!universe->ContainsSourceClient(&client));
  CPPUNIT_ASSERT(!universe->ContainsSinkClient(&client));
  CPPUNIT_ASSERT(!universe->IsActive());

  // try to remove it again
  CPPUNIT_ASSERT(!universe->RemoveSourceClient(&client));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SourceClientCount());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SinkClientCount());
  CPPUNIT_ASSERT(!universe->ContainsSourceClient(&client));
  CPPUNIT_ASSERT(!universe->ContainsSinkClient(&client));
  CPPUNIT_ASSERT(!universe->IsActive());
}


/*
 * Check that we can add/remove sink clients from this universes
 */
void UniverseTest::testSinkClients() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SourceClientCount());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SinkClientCount());

  // test that we can add a source client
  MockClient client;
  universe->AddSinkClient(&client);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, universe->SinkClientCount());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SourceClientCount());
  CPPUNIT_ASSERT(universe->ContainsSinkClient(&client));
  CPPUNIT_ASSERT(!universe->ContainsSourceClient(&client));
  CPPUNIT_ASSERT(universe->IsActive());

  // Setting DMX now should update the client
  CPPUNIT_ASSERT(!client.m_dmx_set);
  universe->SetDMX(m_buffer);
  CPPUNIT_ASSERT(client.m_dmx_set);

  // now remove it
  universe->RemoveSinkClient(&client);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SinkClientCount());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SourceClientCount());
  CPPUNIT_ASSERT(!universe->ContainsSinkClient(&client));
  CPPUNIT_ASSERT(!universe->ContainsSourceClient(&client));
  CPPUNIT_ASSERT(!universe->IsActive());

  // try to remove it again
  CPPUNIT_ASSERT(!universe->RemoveSinkClient(&client));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SinkClientCount());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SourceClientCount());
  CPPUNIT_ASSERT(!universe->ContainsSinkClient(&client));
  CPPUNIT_ASSERT(!universe->ContainsSourceClient(&client));
  CPPUNIT_ASSERT(!universe->IsActive());
}


/*
 * Check that LTP merging works correctly
 */
void UniverseTest::testLtpMerging() {
  DmxBuffer buffer1, buffer2, htp_buffer;
  buffer1.SetFromString("1,0,0,10");
  buffer2.SetFromString("0,255,0,5,6,7");

  ola::PortManager port_manager(m_store);

  TimeStamp time_stamp;
  MockDevice device(NULL, "foo");
  MockDevice device2(NULL, "bar");
  TestMockInputPort port(&device, 1, &time_stamp);  // input port
  TestMockInputPort port2(&device2, 1, &time_stamp);  // input port
  port_manager.PatchPort(&port, TEST_UNIVERSE);
  port_manager.PatchPort(&port2, TEST_UNIVERSE);

  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);
  universe->SetMergeMode(Universe::MERGE_LTP);

  CPPUNIT_ASSERT_EQUAL(universe->InputPortCount(), (unsigned int) 2);
  CPPUNIT_ASSERT_EQUAL(universe->OutputPortCount(), (unsigned int) 0);
  CPPUNIT_ASSERT(universe->IsActive());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->GetDMX().Size());

  // Setup the ports with some data, and check that signalling the universe
  // works.
  Clock::CurrentTime(&time_stamp);
  port.WriteDMX(buffer1);
  port.DmxChanged();
  CPPUNIT_ASSERT_EQUAL(ola::DmxSource::PRIORITY_DEFAULT,
                       universe->ActivePriority());
  CPPUNIT_ASSERT_EQUAL(buffer1.Size(), universe->GetDMX().Size());
  CPPUNIT_ASSERT(buffer1 == universe->GetDMX());

  // Now the second port gets data
  Clock::CurrentTime(&time_stamp);
  port2.WriteDMX(buffer2);
  port2.DmxChanged();
  CPPUNIT_ASSERT_EQUAL(ola::DmxSource::PRIORITY_DEFAULT,
                       universe->ActivePriority());
  CPPUNIT_ASSERT_EQUAL(buffer2.Size(), universe->GetDMX().Size());
  CPPUNIT_ASSERT(buffer2 == universe->GetDMX());

  // now resend the first port
  Clock::CurrentTime(&time_stamp);
  port.WriteDMX(buffer1);
  port.DmxChanged();
  CPPUNIT_ASSERT_EQUAL(ola::DmxSource::PRIORITY_DEFAULT,
                       universe->ActivePriority());
  CPPUNIT_ASSERT_EQUAL(buffer1.Size(), universe->GetDMX().Size());
  CPPUNIT_ASSERT(buffer1 == universe->GetDMX());

  // now check a client
  DmxBuffer client_buffer;
  client_buffer.SetFromString("255,0,0,255,10");
  Clock::CurrentTime(&time_stamp);
  ola::DmxSource source(client_buffer, time_stamp,
                        ola::DmxSource::PRIORITY_DEFAULT);
  MockClient input_client;
  input_client.DMXRecieved(TEST_UNIVERSE, source);
  universe->SourceClientDataChanged(&input_client);

  DmxBuffer client_htp_merge_result;
  client_htp_merge_result.SetFromString("255,255,0,255,10,7");
  CPPUNIT_ASSERT_EQUAL(ola::DmxSource::PRIORITY_DEFAULT,
                       universe->ActivePriority());
  CPPUNIT_ASSERT_EQUAL(client_buffer.Size(), universe->GetDMX().Size());
  CPPUNIT_ASSERT(client_buffer == universe->GetDMX());

  // clean up
  universe->RemoveSourceClient(&input_client);
  universe->RemovePort(&port);
  universe->RemovePort(&port2);
  CPPUNIT_ASSERT(!universe->IsActive());
}


/*
 * Check that HTP merging works correctly
 */
void UniverseTest::testHtpMerging() {
  DmxBuffer buffer1, buffer2, htp_buffer;
  buffer1.SetFromString("1,0,0,10");
  buffer2.SetFromString("0,255,0,5,6,7");
  htp_buffer.SetFromString("1,255,0,10,6,7");

  ola::PortManager port_manager(m_store);

  TimeStamp time_stamp;
  MockDevice device(NULL, "foo");
  MockDevice device2(NULL, "bar");
  TestMockInputPort port(&device, 1, &time_stamp);  // input port
  TestMockInputPort port2(&device2, 1, &time_stamp);  // input port
  port_manager.PatchPort(&port, TEST_UNIVERSE);
  port_manager.PatchPort(&port2, TEST_UNIVERSE);

  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);
  universe->SetMergeMode(Universe::MERGE_HTP);

  CPPUNIT_ASSERT_EQUAL(universe->InputPortCount(), (unsigned int) 2);
  CPPUNIT_ASSERT_EQUAL(universe->OutputPortCount(), (unsigned int) 0);
  CPPUNIT_ASSERT(universe->IsActive());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->GetDMX().Size());

  // Setup the ports with some data, and check that signalling the universe
  // works.
  Clock::CurrentTime(&time_stamp);
  port.WriteDMX(buffer1);
  port.DmxChanged();
  CPPUNIT_ASSERT_EQUAL(ola::DmxSource::PRIORITY_DEFAULT,
                       universe->ActivePriority());
  CPPUNIT_ASSERT_EQUAL(buffer1.Size(), universe->GetDMX().Size());
  CPPUNIT_ASSERT(buffer1 == universe->GetDMX());

  // Now the second port gets data
  Clock::CurrentTime(&time_stamp);
  port2.WriteDMX(buffer2);
  port2.DmxChanged();
  CPPUNIT_ASSERT_EQUAL(ola::DmxSource::PRIORITY_DEFAULT,
                       universe->ActivePriority());
  CPPUNIT_ASSERT_EQUAL(htp_buffer.Size(), universe->GetDMX().Size());
  CPPUNIT_ASSERT(htp_buffer == universe->GetDMX());

  // now raise the priority of the second port
  uint8_t new_priority = 120;
  port2.SetPriority(new_priority);
  Clock::CurrentTime(&time_stamp);
  port2.DmxChanged();
  CPPUNIT_ASSERT_EQUAL(new_priority, universe->ActivePriority());
  CPPUNIT_ASSERT_EQUAL(buffer2.Size(), universe->GetDMX().Size());
  CPPUNIT_ASSERT(buffer2 == universe->GetDMX());

  // raise the priority of the first port
  port.SetPriority(new_priority);
  Clock::CurrentTime(&time_stamp);
  port.DmxChanged();
  CPPUNIT_ASSERT_EQUAL(new_priority, universe->ActivePriority());
  CPPUNIT_ASSERT_EQUAL(htp_buffer.Size(), universe->GetDMX().Size());
  CPPUNIT_ASSERT(htp_buffer == universe->GetDMX());

  // now check a client
  DmxBuffer client_buffer;
  client_buffer.SetFromString("255,0,0,255,10");
  Clock::CurrentTime(&time_stamp);
  ola::DmxSource source(client_buffer, time_stamp, new_priority);
  MockClient input_client;
  input_client.DMXRecieved(TEST_UNIVERSE, source);
  universe->SourceClientDataChanged(&input_client);

  DmxBuffer client_htp_merge_result;
  client_htp_merge_result.SetFromString("255,255,0,255,10,7");
  CPPUNIT_ASSERT_EQUAL(new_priority, universe->ActivePriority());
  CPPUNIT_ASSERT_EQUAL(client_htp_merge_result.Size(),
                       universe->GetDMX().Size());
  CPPUNIT_ASSERT(client_htp_merge_result == universe->GetDMX());

  // clean up
  universe->RemoveSourceClient(&input_client);
  universe->RemovePort(&port);
  universe->RemovePort(&port2);
  CPPUNIT_ASSERT(!universe->IsActive());
}
