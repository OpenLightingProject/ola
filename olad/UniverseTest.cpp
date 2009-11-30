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

#include "ola/DmxBuffer.h"
#include "olad/Client.h"
#include "olad/Port.h"
#include "olad/Preferences.h"
#include "olad/Universe.h"
#include "olad/UniverseStore.h"
#include "olad/TestCommon.h"

using ola::AbstractDevice;
using ola::DmxBuffer;
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

    void SetupUniverseWithPorts(Universe **universe,
                                TestMockInputPort **input,
                                TestMockOutputPort **output);
    void CleanUpUniverseWithPorts(Universe *universe,
                                  TestMockInputPort *input,
                                  TestMockOutputPort *output);
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
  m_preferences = new ola::MemoryPreferences("foo");
  m_store = new ola::UniverseStore(m_preferences, NULL);
  m_buffer.Set(TEST_DATA);
}


void UniverseTest::tearDown() {
  delete m_store;
  delete m_preferences;
}

void UniverseTest::SetupUniverseWithPorts(
    Universe **universe,
    TestMockInputPort **input,
    TestMockOutputPort **output) {
  // Setup an input port and client to HTP merge between, and a output port to
  // hold the result
  *input = new TestMockInputPort(NULL, 1);
  *output = new TestMockOutputPort(NULL, 2);  // output port
  *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);
  (*universe)->AddPort(*input);
  (*universe)->AddPort(*output);
}

void UniverseTest::CleanUpUniverseWithPorts(
    Universe *universe,
    TestMockInputPort *input,
    TestMockOutputPort *output) {
  universe->RemovePort(input);
  universe->RemovePort(output);
  delete input;
  delete output;
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
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);

  TestMockInputPort port(NULL, 1);  // input port
  universe->AddPort(&port);
  CPPUNIT_ASSERT_EQUAL(universe->InputPortCount(), (unsigned int) 1);
  CPPUNIT_ASSERT_EQUAL(universe->OutputPortCount(), (unsigned int) 0);
  CPPUNIT_ASSERT(universe->IsActive());

  // Setup the port with some data, and check that signalling the universe
  // works.
  port.WriteDMX(m_buffer);
  universe->PortDataChanged(&port);
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
  const string input_port_data = "aaabbbccc";
  const string input_client_data = "baaabcabdabc";
  const string set_dmx_data = "aafbeb";

  // Setup an input port and client and a output port to hold the result
  TestMockInputPort input_port(NULL, 1);  // input port
  input_port.WriteDMX(DmxBuffer(input_port_data));
  DmxBuffer input_client_buffer(input_port_data);
  MockClient input_client;
  input_client.DMXRecieved(TEST_UNIVERSE, input_client_buffer);
  TestMockOutputPort output_port(NULL, 2);  // output port

  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  CPPUNIT_ASSERT(universe);
  universe->SetMergeMode(Universe::MERGE_LTP);

  universe->AddPort(&input_port);
  universe->AddPort(&output_port);
  universe->PortDataChanged(&input_port);
  CPPUNIT_ASSERT(input_port.ReadDMX() == output_port.ReadDMX());

  // Now the client gets some data:
  universe->SourceClientDataChanged(&input_client);
  CPPUNIT_ASSERT(input_client_buffer == output_port.ReadDMX());

  // Now check SetDMX merges as well
  CPPUNIT_ASSERT(universe->SetDMX(DmxBuffer(set_dmx_data)));
  CPPUNIT_ASSERT(DmxBuffer(set_dmx_data) == output_port.ReadDMX());

  universe->RemoveSourceClient(&input_client);
  universe->RemovePort(&output_port);
  universe->RemovePort(&input_port);
}


/*
 * Check that HTP merging works correctly
 */
void UniverseTest::testHtpMerging() {
  const string input_port_data = "aaabbbccc";
  const string input_client_data = "baaabcabdabc";
  const string merge_result_data = "baabbcccdabc";
  const string set_dmx_data = "aafbeb";
  const string merge_result_data2 = "bafbecccdabc";

  Universe *universe;
  TestMockInputPort *input_port;
  TestMockOutputPort *output_port;
  SetupUniverseWithPorts(&universe, &input_port, &output_port);
  input_port->WriteDMX(DmxBuffer(input_port_data));
  universe->SetMergeMode(Universe::MERGE_HTP);
  universe->PortDataChanged(input_port);
  CPPUNIT_ASSERT(input_port->ReadDMX() == output_port->ReadDMX());

  // Now the client gets some data:
  MockClient input_client;
  input_client.DMXRecieved(TEST_UNIVERSE, DmxBuffer(input_client_data));
  universe->SourceClientDataChanged(&input_client);
  CPPUNIT_ASSERT(DmxBuffer(merge_result_data) == output_port->ReadDMX());

  // Now check SetDMX merges as well
  CPPUNIT_ASSERT(universe->SetDMX(DmxBuffer(set_dmx_data)));
  CPPUNIT_ASSERT(DmxBuffer(merge_result_data2) == output_port->ReadDMX());

  universe->RemoveSourceClient(&input_client);
  CleanUpUniverseWithPorts(universe, input_port, output_port);
  CPPUNIT_ASSERT(!universe->IsActive());
  m_store->GarbageCollectUniverses();

  // Check the case where the input port isn't initialized
  SetupUniverseWithPorts(&universe, &input_port, &output_port);
  universe->SetMergeMode(Universe::MERGE_HTP);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, output_port->ReadDMX().Size());

  // Now the client gets some data
  universe->SourceClientDataChanged(&input_client);
  CPPUNIT_ASSERT(DmxBuffer(input_client_data) == output_port->ReadDMX());

  // And now some different data
  input_client.DMXRecieved(TEST_UNIVERSE,  DmxBuffer(input_port_data));
  universe->SourceClientDataChanged(&input_client);
  CPPUNIT_ASSERT(DmxBuffer(input_port_data) == output_port->ReadDMX());

  universe->RemoveSourceClient(&input_client);
  CleanUpUniverseWithPorts(universe, input_port, output_port);
  CPPUNIT_ASSERT(!universe->IsActive());
  m_store->GarbageCollectUniverses();
}
