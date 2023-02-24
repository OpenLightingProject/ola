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
 * OlaServerServiceImplTest.cpp
 * Test fixture for the OlaServerServiceImpl class
 * Copyright (C) 2005 Simon Newton
 *
 * How this file is organized:
 *  We test each rpc method in OlaServerServiceImpl, for each method, we have a
 *  series of Check objects which validate the rpc response.
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "common/rpc/RpcController.h"
#include "common/rpc/RpcSession.h"
#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/Constants.h"
#include "ola/DmxBuffer.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"
#include "olad/OlaServerServiceImpl.h"
#include "olad/PluginLoader.h"
#include "olad/Universe.h"
#include "olad/plugin_api/Client.h"
#include "olad/plugin_api/DeviceManager.h"
#include "olad/plugin_api/UniverseStore.h"

using ola::Client;
using ola::NewSingleCallback;
using ola::SingleUseCallback0;
using ola::DmxBuffer;
using ola::OlaServerServiceImpl;
using ola::Universe;
using ola::UniverseStore;
using ola::rpc::RpcController;
using ola::rpc::RpcSession;
using std::string;

class OlaServerServiceImplTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(OlaServerServiceImplTest);
  CPPUNIT_TEST(testGetDmx);
  CPPUNIT_TEST(testRegisterForDmx);
  CPPUNIT_TEST(testUpdateDmxData);
  CPPUNIT_TEST(testSetUniverseName);
  CPPUNIT_TEST(testSetMergeMode);
  CPPUNIT_TEST_SUITE_END();

 public:
    OlaServerServiceImplTest():
      m_uid(ola::OPEN_LIGHTING_ESTA_CODE, 0) {
    }

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

    void testGetDmx();
    void testRegisterForDmx();
    void testUpdateDmxData();
    void testSetUniverseName();
    void testSetMergeMode();

 private:
    ola::rdm::UID m_uid;
    ola::Clock m_clock;

    void CallGetDmx(OlaServerServiceImpl *service,
                    int universe_id,
                    class GetDmxCheck *check);
    void CallRegisterForDmx(OlaServerServiceImpl *service,
                            int universe_id,
                            ola::proto::RegisterAction action,
                            class RegisterForDmxCheck *check);
    void CallUpdateDmxData(OlaServerServiceImpl *service,
                           Client *client,
                           int universe_id,
                           const DmxBuffer &data,
                           class UpdateDmxDataCheck *check);
    void CallSetUniverseName(OlaServerServiceImpl *service,
                             int universe_id,
                             const string &name,
                             class SetUniverseNameCheck *check);
    void CallSetMergeMode(OlaServerServiceImpl *service,
                          int universe_id,
                          ola::proto::MergeMode merge_mode,
                          class SetMergeModeCheck *check);
};

CPPUNIT_TEST_SUITE_REGISTRATION(OlaServerServiceImplTest);

static const uint8_t SAMPLE_DMX_DATA[] = {1, 2, 3, 4, 5};

/*
 * The GetDmx Checks
 */
class GetDmxCheck {
 public:
  virtual ~GetDmxCheck() {}
  virtual void Check(RpcController *controller,
                     ola::proto::DmxData *reply) = 0;
};


/*
 * Assert that the data is all 0
 */
class GetDmxNoDataCheck: public GetDmxCheck {
 public:
  void Check(RpcController *controller,
             ola::proto::DmxData *reply) {
    DmxBuffer empty_buffer;
    OLA_ASSERT_FALSE(controller->Failed());
    OLA_ASSERT_EQ(empty_buffer, DmxBuffer(reply->data()));
  }
};


/*
 * Assert that the data matches the test data.
 */
class GetDmxValidDataCheck: public GetDmxCheck {
 public:
  void Check(RpcController *controller,
             ola::proto::DmxData *reply) {
    OLA_ASSERT_FALSE(controller->Failed());
    OLA_ASSERT_EQ(
        DmxBuffer(SAMPLE_DMX_DATA, sizeof(SAMPLE_DMX_DATA)),
        DmxBuffer(reply->data()));
  }
};


/*
 * RegisterForDmxChecks
 */
class RegisterForDmxCheck {
 public:
  virtual ~RegisterForDmxCheck() {}
  virtual void Check(RpcController *controller, ola::proto::Ack *reply) = 0;
};


/*
 * UpdateDmxDataCheck
 */
class UpdateDmxDataCheck {
 public:
  virtual ~UpdateDmxDataCheck() {}
  virtual void Check(RpcController *controller, ola::proto::Ack *reply) = 0;
};


/*
 * SetUniverseNameCheck
 */
class SetUniverseNameCheck {
 public:
  virtual ~SetUniverseNameCheck() {}
  virtual void Check(RpcController *controller, ola::proto::Ack *reply) = 0;
};


/*
 * SetMergeModeCheck
 */
class SetMergeModeCheck {
 public:
  virtual ~SetMergeModeCheck() {}
  virtual void Check(RpcController *controller, ola::proto::Ack *reply) = 0;
};


/*
 * Assert that we got a missing universe error
 */
template<typename parent, typename reply>
class GenericMissingUniverseCheck: public parent {
 public:
  void Check(RpcController *controller, OLA_UNUSED reply *r) {
    OLA_ASSERT(controller->Failed());
    OLA_ASSERT_EQ(string("Universe doesn't exist"), controller->ErrorText());
  }
};


/*
 * Assert that we got an ack
 */
template<typename parent>
class GenericAckCheck: public parent {
 public:
  void Check(RpcController *controller, OLA_UNUSED ola::proto::Ack *r) {
    OLA_ASSERT_FALSE(controller->Failed());
  }
};


/*
 * Check that the GetDmx method works
 */
void OlaServerServiceImplTest::testGetDmx() {
  UniverseStore store(NULL, NULL);
  OlaServerServiceImpl service(&store, NULL, NULL, NULL, NULL, NULL, NULL);

  GenericMissingUniverseCheck<GetDmxCheck, ola::proto::DmxData>
    missing_universe_check;
  GetDmxNoDataCheck empty_data_check;
  GetDmxValidDataCheck valid_data_check;

  // test a universe that doesn't exist
  unsigned int universe_id = 0;
  CallGetDmx(&service, universe_id, &missing_universe_check);

  // test a new universe
  Universe *universe = store.GetUniverseOrCreate(universe_id);
  OLA_ASSERT_NOT_NULL(universe);
  CallGetDmx(&service, universe_id, &empty_data_check);

  // Set the universe data
  DmxBuffer buffer(SAMPLE_DMX_DATA, sizeof(SAMPLE_DMX_DATA));
  universe->SetDMX(buffer);
  CallGetDmx(&service, universe_id, &valid_data_check);

  // remove the universe and try again
  store.AddUniverseGarbageCollection(universe);
  store.GarbageCollectUniverses();
  CallGetDmx(&service, universe_id, &missing_universe_check);
}

/*
 * Call the GetDmx method
 * @param impl the OlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param check the GetDmxCheck class to use for the callback check
 */
void OlaServerServiceImplTest::CallGetDmx(OlaServerServiceImpl *service,
                                          int universe_id,
                                          GetDmxCheck *check) {
  RpcSession session(NULL);
  RpcController controller(&session);
  ola::proto::UniverseRequest request;
  ola::proto::DmxData response;

  SingleUseCallback0<void> *closure = NewSingleCallback(
      check,
      &GetDmxCheck::Check,
      &controller,
      &response);

  request.set_universe(universe_id);
  service->GetDmx(&controller, &request, &response, closure);
}


/*
 * Check the RegisterForDmx method works
 */
void OlaServerServiceImplTest::testRegisterForDmx() {
  UniverseStore store(NULL, NULL);
  OlaServerServiceImpl service(&store, NULL, NULL, NULL, NULL, NULL, NULL);

  // Register for a universe that doesn't exist
  unsigned int universe_id = 0;
  unsigned int second_universe_id = 99;
  GenericAckCheck<RegisterForDmxCheck> ack_check;
  CallRegisterForDmx(&service, universe_id, ola::proto::REGISTER, &ack_check);

  // The universe should exist now and the client should be bound
  Universe *universe = store.GetUniverse(universe_id);
  OLA_ASSERT_NOT_NULL(universe);
  OLA_ASSERT(universe->ContainsSinkClient(NULL));
  OLA_ASSERT_EQ((unsigned int) 1, universe->SinkClientCount());

  // Try to register again
  CallRegisterForDmx(&service, universe_id, ola::proto::REGISTER, &ack_check);
  OLA_ASSERT(universe->ContainsSinkClient(NULL));
  OLA_ASSERT_EQ((unsigned int) 1, universe->SinkClientCount());

  // Register a second universe
  CallRegisterForDmx(&service, second_universe_id, ola::proto::REGISTER,
                     &ack_check);
  Universe *second_universe = store.GetUniverse(universe_id);
  OLA_ASSERT(second_universe->ContainsSinkClient(NULL));
  OLA_ASSERT_EQ((unsigned int) 1, second_universe->SinkClientCount());

  // Unregister the first universe
  CallRegisterForDmx(&service, universe_id, ola::proto::UNREGISTER, &ack_check);
  OLA_ASSERT_FALSE(universe->ContainsSinkClient(NULL));
  OLA_ASSERT_EQ((unsigned int) 0, universe->SinkClientCount());

  // Unregister the second universe
  CallRegisterForDmx(&service, second_universe_id, ola::proto::UNREGISTER,
                     &ack_check);
  OLA_ASSERT_FALSE(second_universe->ContainsSinkClient(NULL));
  OLA_ASSERT_EQ((unsigned int) 0, second_universe->SinkClientCount());

  // Unregister again
  CallRegisterForDmx(&service, universe_id, ola::proto::UNREGISTER, &ack_check);
  OLA_ASSERT_FALSE(universe->ContainsSinkClient(NULL));
  OLA_ASSERT_EQ((unsigned int) 0, universe->SinkClientCount());
}


/*
 * Call the RegisterForDmx method
 * @param impl the OlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param action the action to use REGISTER or UNREGISTER
 * @param check the RegisterForDmxCheck to use for the callback check
 */
void OlaServerServiceImplTest::CallRegisterForDmx(
    OlaServerServiceImpl *service,
    int universe_id,
    ola::proto::RegisterAction action,
    RegisterForDmxCheck *check) {
  RpcSession session(NULL);
  RpcController controller(&session);
  ola::proto::RegisterDmxRequest request;
  ola::proto::Ack response;

  SingleUseCallback0<void> *closure = NewSingleCallback(
      check,
      &RegisterForDmxCheck::Check,
      &controller,
      &response);

  request.set_universe(universe_id);
  request.set_action(action);
  service->RegisterForDmx(&controller, &request, &response, closure);
}


/*
 * Check the UpdateDmxData method works
 */
void OlaServerServiceImplTest::testUpdateDmxData() {
  UniverseStore store(NULL, NULL);
  ola::TimeStamp time1;
  ola::Client client1(NULL, m_uid);
  ola::Client client2(NULL, m_uid);
  OlaServerServiceImpl service(&store, NULL, NULL, NULL, NULL,
                               &time1, NULL);

  GenericMissingUniverseCheck<UpdateDmxDataCheck, ola::proto::Ack>
    missing_universe_check;
  GenericAckCheck<UpdateDmxDataCheck> ack_check;
  unsigned int universe_id = 0;
  DmxBuffer dmx_data("this is a test");
  DmxBuffer dmx_data2("different data hmm");

  // Update a universe that doesn't exist
  m_clock.CurrentMonotonicTime(&time1);
  CallUpdateDmxData(&service, &client1, universe_id, dmx_data,
                    &missing_universe_check);
  Universe *universe = store.GetUniverse(universe_id);
  OLA_ASSERT_FALSE(universe);

  // Update a universe that exists
  m_clock.CurrentMonotonicTime(&time1);
  universe = store.GetUniverseOrCreate(universe_id);
  CallUpdateDmxData(&service, &client1, universe_id, dmx_data, &ack_check);
  OLA_ASSERT_EQ(dmx_data, universe->GetDMX());

  // Update a second client with an older timestamp
  // make sure we're in ltp mode
  OLA_ASSERT_EQ(universe->MergeMode(), Universe::MERGE_LTP);
  time1 = time1 - ola::TimeInterval(1000000);
  CallUpdateDmxData(&service, &client2, universe_id, dmx_data2, &ack_check);
  OLA_ASSERT_EQ(dmx_data.Size(), universe->GetDMX().Size());
  // Should continue to hold the old data
  OLA_ASSERT_EQ(dmx_data, universe->GetDMX());

  // Now send a new update
  m_clock.CurrentMonotonicTime(&time1);
  CallUpdateDmxData(&service, &client2, universe_id, dmx_data2, &ack_check);
  OLA_ASSERT_EQ(dmx_data2, universe->GetDMX());
}

/*
 * Call the UpdateDmxDataCheck method
 * @param impl the OlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param data the DmxBuffer to use as data
 * @param check the SetUniverseNameCheck to use for the callback check
 */
void OlaServerServiceImplTest::CallUpdateDmxData(
    OlaServerServiceImpl *service,
    Client *client,
    int universe_id,
    const DmxBuffer &data,
    UpdateDmxDataCheck *check) {
  RpcSession session(NULL);
  session.SetData(client);
  RpcController controller(&session);
  ola::proto::DmxData request;
  ola::proto::Ack response;
  SingleUseCallback0<void> *closure = NewSingleCallback(
      check,
      &UpdateDmxDataCheck::Check,
      &controller,
      &response);

  request.set_universe(universe_id);
  request.set_data(data.Get());
  service->UpdateDmxData(&controller, &request, &response, closure);
}

/*
 * Check the SetUniverseName method works
 */
void OlaServerServiceImplTest::testSetUniverseName() {
  UniverseStore store(NULL, NULL);
  OlaServerServiceImpl service(&store, NULL, NULL, NULL, NULL, NULL, NULL);

  unsigned int universe_id = 0;
  string universe_name = "test 1";
  string universe_name2 = "test 1-2";

  GenericAckCheck<SetUniverseNameCheck> ack_check;
  GenericMissingUniverseCheck<SetUniverseNameCheck, ola::proto::Ack>
    missing_universe_check;

  // Check we get an error for a missing universe
  CallSetUniverseName(&service, universe_id, universe_name,
                      &missing_universe_check);
  Universe *universe = store.GetUniverse(universe_id);
  OLA_ASSERT_FALSE(universe);

  // Check SetUniverseName works on an existing universe
  universe = store.GetUniverseOrCreate(universe_id);
  CallSetUniverseName(&service, universe_id, universe_name, &ack_check);
  OLA_ASSERT_EQ(universe_name, universe->Name());

  // Run it again with a new name
  CallSetUniverseName(&service, universe_id, universe_name2, &ack_check);
  OLA_ASSERT_EQ(universe_name2, universe->Name());
}

/*
 * Call the SetUniverseName method
 * @param impl the OlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param name the name to use
 * @param check the SetUniverseNameCheck to use for the callback check
 */
void OlaServerServiceImplTest::CallSetUniverseName(
    OlaServerServiceImpl *service,
    int universe_id,
    const string &name,
    SetUniverseNameCheck *check) {
  RpcSession session(NULL);
  RpcController controller(&session);
  ola::proto::UniverseNameRequest request;
  ola::proto::Ack response;
  SingleUseCallback0<void> *closure = NewSingleCallback(
      check,
      &SetUniverseNameCheck::Check,
      &controller,
      &response);

  request.set_universe(universe_id);
  request.set_name(name);
  service->SetUniverseName(&controller, &request, &response, closure);
}

/*
 * Check the SetMergeMode method works
 */
void OlaServerServiceImplTest::testSetMergeMode() {
  UniverseStore store(NULL, NULL);
  OlaServerServiceImpl service(&store, NULL, NULL, NULL, NULL, NULL, NULL);

  unsigned int universe_id = 0;

  GenericAckCheck<SetMergeModeCheck> ack_check;
  GenericMissingUniverseCheck<SetMergeModeCheck, ola::proto::Ack>
    missing_universe_check;

  // Check we get an error for a missing universe
  CallSetMergeMode(&service, universe_id, ola::proto::HTP,
                   &missing_universe_check);
  Universe *universe = store.GetUniverse(universe_id);
  OLA_ASSERT_FALSE(universe);

  // Check SetUniverseName works
  universe = store.GetUniverseOrCreate(universe_id);
  CallSetMergeMode(&service, universe_id, ola::proto::HTP, &ack_check);
  OLA_ASSERT_EQ(Universe::MERGE_HTP, universe->MergeMode());

  // Run it again
  CallSetMergeMode(&service, universe_id, ola::proto::LTP, &ack_check);
  OLA_ASSERT_EQ(Universe::MERGE_LTP, universe->MergeMode());
}

/*
 * Call the SetMergeMode method
 * @param impl the OlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param mode the merge_mode to use
 * @param check the SetMergeModeCheck to use for the callback check
*/
void OlaServerServiceImplTest::CallSetMergeMode(
    OlaServerServiceImpl *service,
    int universe_id,
    ola::proto::MergeMode merge_mode,
    SetMergeModeCheck *check) {
  RpcSession session(NULL);
  RpcController controller(&session);
  ola::proto::MergeModeRequest request;
  ola::proto::Ack response;
  ola::SingleUseCallback0<void> *closure = NewSingleCallback(
      check,
      &SetMergeModeCheck::Check,
      &controller,
      &response);

  request.set_universe(universe_id);
  request.set_merge_mode(merge_mode);
  service->SetMergeMode(&controller, &request, &response, closure);
}
