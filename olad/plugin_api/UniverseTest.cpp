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
 * UniverseTest.cpp
 * Test fixture for the Universe and UniverseStore classes
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <iostream>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/Clock.h"
#include "ola/DmxBuffer.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMReply.h"
#include "ola/rdm/RDMResponseCodes.h"
#include "ola/rdm/UID.h"
#include "olad/DmxSource.h"
#include "olad/PluginAdaptor.h"
#include "olad/Port.h"
#include "olad/PortBroker.h"
#include "olad/Preferences.h"
#include "olad/Universe.h"
#include "olad/plugin_api/Client.h"
#include "olad/plugin_api/PortManager.h"
#include "olad/plugin_api/TestCommon.h"
#include "olad/plugin_api/UniverseStore.h"
#include "ola/testing/TestUtils.h"


using ola::AbstractDevice;
using ola::Clock;
using ola::DmxBuffer;
using ola::NewCallback;
using ola::NewSingleCallback;
using ola::TimeStamp;
using ola::Universe;
using ola::rdm::NewDiscoveryUniqueBranchRequest;
using ola::rdm::RDMCallback;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::rdm::RDMStatusCode;
using std::string;
using std::vector;

static unsigned int TEST_UNIVERSE = 1;
static const char TEST_DATA[] = "this is some test data";


class UniverseTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UniverseTest);
  CPPUNIT_TEST(testLifecycle);
  CPPUNIT_TEST(testSetGetDmx);
  CPPUNIT_TEST(testSendDmx);
  CPPUNIT_TEST(testReceiveDmx);
  CPPUNIT_TEST(testSourceClients);
  CPPUNIT_TEST(testSinkClients);
  CPPUNIT_TEST(testLtpMerging);
  CPPUNIT_TEST(testHtpMerging);
  CPPUNIT_TEST(testRDMDiscovery);
  CPPUNIT_TEST(testRDMSend);
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();
  void testLifecycle();
  void testSetGetDmx();
  void testSendDmx();
  void testReceiveDmx();
  void testSourceClients();
  void testSinkClients();
  void testLtpMerging();
  void testHtpMerging();
  void testRDMDiscovery();
  void testRDMSend();

 private:
  ola::MemoryPreferences *m_preferences;
  ola::UniverseStore *m_store;
  DmxBuffer m_buffer;
  ola::Clock m_clock;

  void ConfirmUIDs(UIDSet *expected, const UIDSet &uids);

  void ConfirmRDM(int line,
                  RDMStatusCode expected_status_code,
                  const RDMResponse *expected_response,
                  RDMReply *reply);

  void ReturnRDMCode(RDMStatusCode status_code,
                     const RDMRequest *request,
                     RDMCallback *callback) {
    delete request;
    RunRDMCallback(callback, status_code);
  }
};


class MockClient: public ola::Client {
 public:
  MockClient()
      : ola::Client(NULL, UID(ola::OPEN_LIGHTING_ESTA_CODE, 0)),
        m_dmx_set(false) {
  }

  bool SendDMX(unsigned int universe_id, uint8_t priority,
               const DmxBuffer &buffer) {
    OLA_ASSERT_EQ(TEST_UNIVERSE, universe_id);
    OLA_ASSERT_EQ(ola::dmx::SOURCE_PRIORITY_MIN, priority);
    OLA_ASSERT_EQ(string(TEST_DATA), buffer.Get());
    m_dmx_set = true;
    return true;
  }
  bool m_dmx_set;
};


CPPUNIT_TEST_SUITE_REGISTRATION(UniverseTest);


void UniverseTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_preferences = new ola::MemoryPreferences("foo");
  m_store = new ola::UniverseStore(m_preferences, NULL);
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
  OLA_ASSERT_FALSE(universe);

  universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  OLA_ASSERT(universe);
  OLA_ASSERT_EQ(TEST_UNIVERSE, universe->UniverseId());
  OLA_ASSERT_EQ((unsigned int) 1, m_store->UniverseCount());
  OLA_ASSERT_EQ(Universe::MERGE_LTP, universe->MergeMode());
  OLA_ASSERT_FALSE(universe->IsActive());

  string universe_name = "New Name";
  universe->SetName(universe_name);
  universe->SetMergeMode(Universe::MERGE_HTP);

  OLA_ASSERT_EQ(universe_name, universe->Name());
  OLA_ASSERT_EQ(Universe::MERGE_HTP, universe->MergeMode());

  // delete it
  m_store->AddUniverseGarbageCollection(universe);
  m_store->GarbageCollectUniverses();
  OLA_ASSERT_EQ((unsigned int) 0, m_store->UniverseCount());
  universe = m_store->GetUniverse(TEST_UNIVERSE);
  OLA_ASSERT_FALSE(universe);

  // now re-create it
  universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  OLA_ASSERT(universe);
  OLA_ASSERT_EQ((unsigned int) 1, m_store->UniverseCount());
  OLA_ASSERT_EQ(TEST_UNIVERSE, universe->UniverseId());
  OLA_ASSERT_EQ(universe_name, universe->Name());
  OLA_ASSERT_EQ(Universe::MERGE_HTP, universe->MergeMode());

  m_store->DeleteAll();
  OLA_ASSERT_EQ((unsigned int) 0, m_store->UniverseCount());
}


/*
 * Check that SetDMX/GetDMX works
 */
void UniverseTest::testSetGetDmx() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  OLA_ASSERT(universe);

  // a new universe should be all 0s
  DmxBuffer empty_buffer;
  OLA_ASSERT_DMX_EQUALS(empty_buffer, universe->GetDMX());

  // check that SetDMX works
  OLA_ASSERT(universe->SetDMX(m_buffer));
  OLA_ASSERT_DMX_EQUALS(m_buffer, universe->GetDMX());
}


/*
 * Check that SendDmx updates all ports
 */
void UniverseTest::testSendDmx() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  OLA_ASSERT(universe);

  TestMockOutputPort port(NULL, 1);  // output port
  universe->AddPort(&port);
  OLA_ASSERT_EQ((unsigned int) 0, universe->InputPortCount());
  OLA_ASSERT_EQ((unsigned int) 1, universe->OutputPortCount());
  OLA_ASSERT(universe->IsActive());

  // send some data to the universe and check the port gets it
  OLA_ASSERT(universe->SetDMX(m_buffer));
  OLA_ASSERT_DMX_EQUALS(m_buffer, port.ReadDMX());

  // remove the port from the universe
  universe->RemovePort(&port);
  OLA_ASSERT_EQ((unsigned int) 0, universe->InputPortCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->OutputPortCount());
  OLA_ASSERT_FALSE(universe->IsActive());
}


/*
 * Check that we update when ports have new data
 */
void UniverseTest::testReceiveDmx() {
  ola::PortBroker broker;
  ola::PortManager port_manager(m_store, &broker);
  TimeStamp time_stamp;
  MockSelectServer ss(&time_stamp);
  ola::PluginAdaptor plugin_adaptor(NULL, &ss, NULL, NULL, NULL, NULL);

  MockDevice device(NULL, "foo");
  TestMockInputPort port(&device, 1, &plugin_adaptor);  // input port
  port_manager.PatchPort(&port, TEST_UNIVERSE);

  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  OLA_ASSERT(universe);

  OLA_ASSERT_EQ((unsigned int) 1, universe->InputPortCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->OutputPortCount());
  OLA_ASSERT(universe->IsActive());

  // Setup the port with some data, and check that signalling the universe
  // works.
  m_clock.CurrentTime(&time_stamp);
  port.WriteDMX(m_buffer);
  port.DmxChanged();
  OLA_ASSERT_EQ(ola::dmx::SOURCE_PRIORITY_DEFAULT, universe->ActivePriority());
  OLA_ASSERT_EQ(m_buffer.Size(), universe->GetDMX().Size());
  OLA_ASSERT_DMX_EQUALS(m_buffer, universe->GetDMX());

  // Remove the port from the universe
  universe->RemovePort(&port);
  OLA_ASSERT_FALSE(universe->IsActive());
  OLA_ASSERT_EQ((unsigned int) 0, universe->InputPortCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->OutputPortCount());
}


/*
 * Check that we can add/remove source clients from this universes
 */
void UniverseTest::testSourceClients() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  OLA_ASSERT(universe);
  OLA_ASSERT_EQ((unsigned int) 0, universe->SourceClientCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->SinkClientCount());

  // test that we can add a source client
  MockClient client;
  universe->AddSourceClient(&client);
  OLA_ASSERT_EQ((unsigned int) 1, universe->SourceClientCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->SinkClientCount());
  OLA_ASSERT(universe->ContainsSourceClient(&client));
  OLA_ASSERT_FALSE(universe->ContainsSinkClient(&client));
  OLA_ASSERT(universe->IsActive());

  // Setting DMX now does nothing
  OLA_ASSERT_FALSE(client.m_dmx_set);
  universe->SetDMX(m_buffer);
  OLA_ASSERT_FALSE(client.m_dmx_set);

  // now remove it
  universe->RemoveSourceClient(&client);
  OLA_ASSERT_EQ((unsigned int) 0, universe->SourceClientCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->SinkClientCount());
  OLA_ASSERT_FALSE(universe->ContainsSourceClient(&client));
  OLA_ASSERT_FALSE(universe->ContainsSinkClient(&client));
  OLA_ASSERT_FALSE(universe->IsActive());

  // try to remove it again
  OLA_ASSERT_FALSE(universe->RemoveSourceClient(&client));
  OLA_ASSERT_EQ((unsigned int) 0, universe->SourceClientCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->SinkClientCount());
  OLA_ASSERT_FALSE(universe->ContainsSourceClient(&client));
  OLA_ASSERT_FALSE(universe->ContainsSinkClient(&client));
  OLA_ASSERT_FALSE(universe->IsActive());
}


/*
 * Check that we can add/remove sink clients from this universes
 */
void UniverseTest::testSinkClients() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  OLA_ASSERT(universe);
  OLA_ASSERT_EQ((unsigned int) 0, universe->SourceClientCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->SinkClientCount());

  // test that we can add a source client
  MockClient client;
  universe->AddSinkClient(&client);
  OLA_ASSERT_EQ((unsigned int) 1, universe->SinkClientCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->SourceClientCount());
  OLA_ASSERT(universe->ContainsSinkClient(&client));
  OLA_ASSERT_FALSE(universe->ContainsSourceClient(&client));
  OLA_ASSERT(universe->IsActive());

  // Setting DMX now should update the client
  OLA_ASSERT_FALSE(client.m_dmx_set);
  universe->SetDMX(m_buffer);
  OLA_ASSERT(client.m_dmx_set);

  // now remove it
  universe->RemoveSinkClient(&client);
  OLA_ASSERT_EQ((unsigned int) 0, universe->SinkClientCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->SourceClientCount());
  OLA_ASSERT_FALSE(universe->ContainsSinkClient(&client));
  OLA_ASSERT_FALSE(universe->ContainsSourceClient(&client));
  OLA_ASSERT_FALSE(universe->IsActive());

  // try to remove it again
  OLA_ASSERT_FALSE(universe->RemoveSinkClient(&client));
  OLA_ASSERT_EQ((unsigned int) 0, universe->SinkClientCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->SourceClientCount());
  OLA_ASSERT_FALSE(universe->ContainsSinkClient(&client));
  OLA_ASSERT_FALSE(universe->ContainsSourceClient(&client));
  OLA_ASSERT_FALSE(universe->IsActive());
}


/*
 * Check that LTP merging works correctly
 */
void UniverseTest::testLtpMerging() {
  DmxBuffer buffer1, buffer2, htp_buffer;
  buffer1.SetFromString("1,0,0,10");
  buffer2.SetFromString("0,255,0,5,6,7");

  ola::PortBroker broker;
  ola::PortManager port_manager(m_store, &broker);

  TimeStamp time_stamp;
  MockSelectServer ss(&time_stamp);
  ola::PluginAdaptor plugin_adaptor(NULL, &ss, NULL, NULL, NULL, NULL);
  MockDevice device(NULL, "foo");
  MockDevice device2(NULL, "bar");
  TestMockInputPort port(&device, 1, &plugin_adaptor);  // input port
  TestMockInputPort port2(&device2, 1, &plugin_adaptor);  // input port
  port_manager.PatchPort(&port, TEST_UNIVERSE);
  port_manager.PatchPort(&port2, TEST_UNIVERSE);

  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  OLA_ASSERT(universe);
  universe->SetMergeMode(Universe::MERGE_LTP);

  OLA_ASSERT_EQ((unsigned int) 2, universe->InputPortCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->OutputPortCount());
  OLA_ASSERT(universe->IsActive());
  OLA_ASSERT_EQ((unsigned int) 0, universe->GetDMX().Size());

  // Setup the ports with some data, and check that signalling the universe
  // works.
  m_clock.CurrentTime(&time_stamp);
  port.WriteDMX(buffer1);
  port.DmxChanged();
  OLA_ASSERT_EQ(ola::dmx::SOURCE_PRIORITY_DEFAULT, universe->ActivePriority());
  OLA_ASSERT_EQ(buffer1.Size(), universe->GetDMX().Size());
  OLA_ASSERT_DMX_EQUALS(buffer1, universe->GetDMX());

  // Now the second port gets data
  m_clock.CurrentTime(&time_stamp);
  port2.WriteDMX(buffer2);
  port2.DmxChanged();
  OLA_ASSERT_EQ(ola::dmx::SOURCE_PRIORITY_DEFAULT, universe->ActivePriority());
  OLA_ASSERT_EQ(buffer2.Size(), universe->GetDMX().Size());
  OLA_ASSERT_DMX_EQUALS(buffer2, universe->GetDMX());

  // now resend the first port
  m_clock.CurrentTime(&time_stamp);
  port.WriteDMX(buffer1);
  port.DmxChanged();
  OLA_ASSERT_EQ(ola::dmx::SOURCE_PRIORITY_DEFAULT, universe->ActivePriority());
  OLA_ASSERT_EQ(buffer1.Size(), universe->GetDMX().Size());
  OLA_ASSERT_DMX_EQUALS(buffer1, universe->GetDMX());

  // now check a client
  DmxBuffer client_buffer;
  client_buffer.SetFromString("255,0,0,255,10");
  m_clock.CurrentTime(&time_stamp);
  ola::DmxSource source(client_buffer, time_stamp,
                        ola::dmx::SOURCE_PRIORITY_DEFAULT);
  MockClient input_client;
  input_client.DMXReceived(TEST_UNIVERSE, source);
  universe->SourceClientDataChanged(&input_client);

  DmxBuffer client_htp_merge_result;
  client_htp_merge_result.SetFromString("255,255,0,255,10,7");
  OLA_ASSERT_EQ(ola::dmx::SOURCE_PRIORITY_DEFAULT, universe->ActivePriority());
  OLA_ASSERT_EQ(client_buffer.Size(), universe->GetDMX().Size());
  OLA_ASSERT_DMX_EQUALS(client_buffer, universe->GetDMX());

  // clean up
  universe->RemoveSourceClient(&input_client);
  universe->RemovePort(&port);
  universe->RemovePort(&port2);
  OLA_ASSERT_FALSE(universe->IsActive());
}


/*
 * Check that HTP merging works correctly
 */
void UniverseTest::testHtpMerging() {
  DmxBuffer buffer1, buffer2, htp_buffer;
  buffer1.SetFromString("1,0,0,10");
  buffer2.SetFromString("0,255,0,5,6,7");
  htp_buffer.SetFromString("1,255,0,10,6,7");

  ola::PortBroker broker;
  ola::PortManager port_manager(m_store, &broker);

  TimeStamp time_stamp;
  MockSelectServer ss(&time_stamp);
  ola::PluginAdaptor plugin_adaptor(NULL, &ss, NULL, NULL, NULL, NULL);
  MockDevice device(NULL, "foo");
  MockDevice device2(NULL, "bar");
  TestMockInputPort port(&device, 1, &plugin_adaptor);  // input port
  TestMockInputPort port2(&device2, 1, &plugin_adaptor);  // input port
  port_manager.PatchPort(&port, TEST_UNIVERSE);
  port_manager.PatchPort(&port2, TEST_UNIVERSE);

  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  OLA_ASSERT(universe);
  universe->SetMergeMode(Universe::MERGE_HTP);

  OLA_ASSERT_EQ(universe->OutputPortCount(), (unsigned int) 0);
  OLA_ASSERT_EQ(universe->OutputPortCount(), (unsigned int) 0);
  OLA_ASSERT(universe->IsActive());
  OLA_ASSERT_EQ((unsigned int) 0, universe->GetDMX().Size());

  // Setup the ports with some data, and check that signalling the universe
  // works.
  m_clock.CurrentTime(&time_stamp);
  port.WriteDMX(buffer1);
  port.DmxChanged();
  OLA_ASSERT_EQ(ola::dmx::SOURCE_PRIORITY_DEFAULT, universe->ActivePriority());
  OLA_ASSERT_EQ(buffer1.Size(), universe->GetDMX().Size());
  OLA_ASSERT_DMX_EQUALS(buffer1, universe->GetDMX());

  // Now the second port gets data
  m_clock.CurrentTime(&time_stamp);
  port2.WriteDMX(buffer2);
  port2.DmxChanged();
  OLA_ASSERT_EQ(ola::dmx::SOURCE_PRIORITY_DEFAULT, universe->ActivePriority());
  OLA_ASSERT_EQ(htp_buffer.Size(), universe->GetDMX().Size());
  OLA_ASSERT_DMX_EQUALS(htp_buffer, universe->GetDMX());

  // now raise the priority of the second port
  uint8_t new_priority = 120;
  port2.SetPriority(new_priority);
  m_clock.CurrentTime(&time_stamp);
  port2.DmxChanged();
  OLA_ASSERT_EQ(new_priority, universe->ActivePriority());
  OLA_ASSERT_EQ(buffer2.Size(), universe->GetDMX().Size());
  OLA_ASSERT_DMX_EQUALS(buffer2, universe->GetDMX());

  // raise the priority of the first port
  port.SetPriority(new_priority);
  m_clock.CurrentTime(&time_stamp);
  port.DmxChanged();
  OLA_ASSERT_EQ(new_priority, universe->ActivePriority());
  OLA_ASSERT_EQ(htp_buffer.Size(), universe->GetDMX().Size());
  OLA_ASSERT_DMX_EQUALS(htp_buffer, universe->GetDMX());

  // now check a client
  DmxBuffer client_buffer;
  client_buffer.SetFromString("255,0,0,255,10");
  m_clock.CurrentTime(&time_stamp);
  ola::DmxSource source(client_buffer, time_stamp, new_priority);
  MockClient input_client;
  input_client.DMXReceived(TEST_UNIVERSE, source);
  universe->SourceClientDataChanged(&input_client);

  DmxBuffer client_htp_merge_result;
  client_htp_merge_result.SetFromString("255,255,0,255,10,7");
  OLA_ASSERT_EQ(new_priority, universe->ActivePriority());
  OLA_ASSERT_EQ(client_htp_merge_result.Size(),
                       universe->GetDMX().Size());
  OLA_ASSERT_DMX_EQUALS(client_htp_merge_result, universe->GetDMX());

  // clean up
  universe->RemoveSourceClient(&input_client);
  universe->RemovePort(&port);
  universe->RemovePort(&port2);
  OLA_ASSERT_FALSE(universe->IsActive());
}


/**
 * Test RDM discovery for a universe/
 */
void UniverseTest::testRDMDiscovery() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  OLA_ASSERT(universe);

  // check the uid set is initially empty
  UIDSet universe_uids;
  universe->GetUIDs(&universe_uids);
  OLA_ASSERT_EQ(0u, universe_uids.Size());

  UID uid1(0x7a70, 1);
  UID uid2(0x7a70, 2);
  UID uid3(0x7a70, 3);
  UIDSet port1_uids, port2_uids;
  port1_uids.AddUID(uid1);
  port2_uids.AddUID(uid2);
  TestMockRDMOutputPort port1(NULL, 1, &port1_uids);
  // this port is configured to update the uids on patch
  TestMockRDMOutputPort port2(NULL, 2, &port2_uids, true);
  universe->AddPort(&port1);
  port1.SetUniverse(universe);
  universe->AddPort(&port2);
  port2.SetUniverse(universe);

  OLA_ASSERT_EQ((unsigned int) 0, universe->InputPortCount());
  OLA_ASSERT_EQ((unsigned int) 2, universe->OutputPortCount());
  universe->GetUIDs(&universe_uids);
  OLA_ASSERT_EQ(1u, universe_uids.Size());
  OLA_ASSERT(universe_uids.Contains(uid2));
  OLA_ASSERT(universe->IsActive());

  // now trigger discovery
  UIDSet expected_uids;
  expected_uids.AddUID(uid1);
  expected_uids.AddUID(uid2);

  universe->RunRDMDiscovery(
    NewSingleCallback(this, &UniverseTest::ConfirmUIDs, &expected_uids),
    true);

  // now add a uid to one port, and remove a uid from another
  port1_uids.AddUID(uid3);
  port2_uids.RemoveUID(uid2);

  expected_uids.AddUID(uid3);
  expected_uids.RemoveUID(uid2);

  universe->RunRDMDiscovery(
    NewSingleCallback(this, &UniverseTest::ConfirmUIDs, &expected_uids),
    true);

  // remove the first port from the universe and confirm there are no more UIDs
  universe->RemovePort(&port1);
  expected_uids.Clear();

  universe->RunRDMDiscovery(
    NewSingleCallback(this, &UniverseTest::ConfirmUIDs, &expected_uids),
    true);

  universe_uids.Clear();
  universe->GetUIDs(&universe_uids);
  OLA_ASSERT_EQ(0u, universe_uids.Size());

  universe->RemovePort(&port2);
  OLA_ASSERT_EQ((unsigned int) 0, universe->InputPortCount());
  OLA_ASSERT_EQ((unsigned int) 0, universe->OutputPortCount());
  OLA_ASSERT_FALSE(universe->IsActive());
}


/**
 * test Sending an RDM request
 */
void UniverseTest::testRDMSend() {
  Universe *universe = m_store->GetUniverseOrCreate(TEST_UNIVERSE);
  OLA_ASSERT(universe);

  // setup the ports with a UID on each
  UID uid1(0x7a70, 1);
  UID uid2(0x7a70, 2);
  UID uid3(0x7a70, 3);
  UIDSet port1_uids, port2_uids;
  port1_uids.AddUID(uid1);
  port2_uids.AddUID(uid2);
  TestMockRDMOutputPort port1(NULL, 1, &port1_uids, true);
  TestMockRDMOutputPort port2(NULL, 2, &port2_uids, true);
  universe->AddPort(&port1);
  port1.SetUniverse(universe);
  universe->AddPort(&port2);
  port2.SetUniverse(universe);

  UID source_uid(0x7a70, 100);
  // first try a command to a uid we don't know about
  RDMRequest *request = new ola::rdm::RDMGetRequest(
      source_uid,
      uid3,
      0,  // transaction #
      1,  // port id
      10,  // sub device
      296,  // param id
      NULL,
      0);

  universe->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &UniverseTest::ConfirmRDM,
                        __LINE__,
                        ola::rdm::RDM_UNKNOWN_UID,
#ifdef __FreeBSD__
                        reinterpret_cast<const RDMResponse*>(0)));
#else
                        reinterpret_cast<const RDMResponse*>(NULL)));
#endif  // __FreeBSD__

  // ok, now try something that returns a response from the port
  request = new ola::rdm::RDMGetRequest(
      source_uid,
      uid1,
      0,  // transaction #
      1,  // port id
      10,  // sub device
      296,  // param id
      NULL,
      0);

  port1.SetRDMHandler(
    NewCallback(this, &UniverseTest::ReturnRDMCode, ola::rdm::RDM_TIMEOUT));

  universe->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &UniverseTest::ConfirmRDM,
                        __LINE__,
                        ola::rdm::RDM_TIMEOUT,
#ifdef __FreeBSD__
                        reinterpret_cast<const RDMResponse*>(0)));
#else
                        reinterpret_cast<const RDMResponse*>(NULL)));
#endif  // __FreeBSD__

  // now try a broadcast fan out
  UID vendorcast_uid = UID::VendorcastAddress(0x7a70);
  request = new ola::rdm::RDMGetRequest(
      source_uid,
      vendorcast_uid,
      0,  // transaction #
      1,  // port id
      10,  // sub device
      296,  // param id
      NULL,
      0);

  port1.SetRDMHandler(
    NewCallback(this,
                &UniverseTest::ReturnRDMCode,
                ola::rdm::RDM_WAS_BROADCAST));
  port2.SetRDMHandler(
    NewCallback(this,
                &UniverseTest::ReturnRDMCode,
                ola::rdm::RDM_WAS_BROADCAST));

  universe->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &UniverseTest::ConfirmRDM,
                        __LINE__,
                        ola::rdm::RDM_WAS_BROADCAST,
#ifdef __FreeBSD__
                        reinterpret_cast<const RDMResponse*>(0)));
#else
                        reinterpret_cast<const RDMResponse*>(NULL)));
#endif  // __FreeBSD__

  // now confirm that if one of the ports fails to send, we see this response
  request = new ola::rdm::RDMGetRequest(
      source_uid,
      vendorcast_uid,
      0,  // transaction #
      1,  // port id
      10,  // sub device
      296,  // param id
      NULL,
      0);

  port2.SetRDMHandler(
    NewCallback(this,
                &UniverseTest::ReturnRDMCode,
                ola::rdm::RDM_FAILED_TO_SEND));

  universe->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &UniverseTest::ConfirmRDM,
                        __LINE__,
                        ola::rdm::RDM_FAILED_TO_SEND,
#ifdef __FreeBSD__
                        reinterpret_cast<const RDMResponse*>(0)));
#else
                        reinterpret_cast<const RDMResponse*>(NULL)));
#endif  // __FreeBSD__

  // DUB responses are slightly different
  request = NewDiscoveryUniqueBranchRequest(source_uid, uid1, uid2, 0);

  port1.SetRDMHandler(
    NewCallback(this,
                &UniverseTest::ReturnRDMCode,
                ola::rdm::RDM_DUB_RESPONSE));
  port2.SetRDMHandler(
    NewCallback(this,
                &UniverseTest::ReturnRDMCode,
                ola::rdm::RDM_DUB_RESPONSE));

  universe->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &UniverseTest::ConfirmRDM,
                        __LINE__,
                        ola::rdm::RDM_DUB_RESPONSE,
#ifdef __FreeBSD__
                        reinterpret_cast<const RDMResponse*>(0)));
#else
                        reinterpret_cast<const RDMResponse*>(NULL)));
#endif  // __FreeBSD__

  // now check that we still get a RDM_DUB_RESPONSE even if one port returns an
  // RDM_TIMEOUT
  request = NewDiscoveryUniqueBranchRequest(source_uid, uid1, uid2, 0);

  port2.SetRDMHandler(
    NewCallback(this,
                &UniverseTest::ReturnRDMCode,
                ola::rdm::RDM_TIMEOUT));

  universe->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &UniverseTest::ConfirmRDM,
                        __LINE__,
                        ola::rdm::RDM_DUB_RESPONSE,
#ifdef __FreeBSD__
                        reinterpret_cast<const RDMResponse*>(0)));
#else
                        reinterpret_cast<const RDMResponse*>(NULL)));
#endif  // __FreeBSD__

  // and the same again but the second port returns
  // RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED
  request = NewDiscoveryUniqueBranchRequest(source_uid, uid1, uid2, 0);

  port2.SetRDMHandler(
    NewCallback(this,
                &UniverseTest::ReturnRDMCode,
                ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED));

  universe->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &UniverseTest::ConfirmRDM,
                        __LINE__,
                        ola::rdm::RDM_DUB_RESPONSE,
#ifdef __FreeBSD__
                        reinterpret_cast<const RDMResponse*>(0)));
#else
                        reinterpret_cast<const RDMResponse*>(NULL)));
#endif  // __FreeBSD__

  // now the first port returns a RDM_TIMEOUT
  request = NewDiscoveryUniqueBranchRequest(source_uid, uid1, uid2, 0);

  port1.SetRDMHandler(
    NewCallback(this,
                &UniverseTest::ReturnRDMCode,
                ola::rdm::RDM_TIMEOUT));

  universe->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &UniverseTest::ConfirmRDM,
                        __LINE__,
                        ola::rdm::RDM_TIMEOUT,
#ifdef __FreeBSD__
                        reinterpret_cast<const RDMResponse*>(0)));
#else
                        reinterpret_cast<const RDMResponse*>(NULL)));
#endif  // __FreeBSD__

  // finally if neither ports support the DUB, we should return that
  request = NewDiscoveryUniqueBranchRequest(source_uid, uid1, uid2, 0);

  port1.SetRDMHandler(
    NewCallback(this,
                &UniverseTest::ReturnRDMCode,
                ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED));

  universe->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &UniverseTest::ConfirmRDM,
                        __LINE__,
                        ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED,
#ifdef __FreeBSD__
                        reinterpret_cast<const RDMResponse*>(0)));
#else
                        reinterpret_cast<const RDMResponse*>(NULL)));
#endif  // __FreeBSD__

  universe->RemovePort(&port1);
  universe->RemovePort(&port2);
}


/**
 * Check we got the uids we expect
 */
void UniverseTest::ConfirmUIDs(UIDSet *expected, const UIDSet &uids) {
  OLA_ASSERT_EQ(*expected, uids);
}


/**
 * Confirm an RDM response
 */
void UniverseTest::ConfirmRDM(int line,
                              RDMStatusCode expected_status_code,
                              const RDMResponse *expected_response,
                              RDMReply *reply) {
  std::ostringstream str;
  str << "Line " << line;
  OLA_ASSERT_EQ_MSG(expected_status_code,
                    reply->StatusCode(),
                    str.str());
  OLA_ASSERT_EQ_MSG(expected_response, reply->Response(), str.str());
}
