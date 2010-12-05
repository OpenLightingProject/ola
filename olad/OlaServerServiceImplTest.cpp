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
 * OlaServerServiceImplTest.cpp
 * Test fixture for the OlaServerServiceImpl class
 * Copyright (C) 2005-2009 Simon Newton
 *
 * How this file is organized:
 *  We test each rpc method in OlaServerServiceImpl, for each method, we have a
 *  series of Check objects which validate the rpc response.
 */

#include <cppunit/extensions/HelperMacros.h>
#include <google/protobuf/stubs/common.h>
#include <string>

#include "common/rpc/SimpleRpcController.h"
#include "ola/BaseTypes.h"
#include "ola/Clock.h"
#include "ola/DmxBuffer.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "olad/Client.h"
#include "olad/DeviceManager.h"
#include "olad/OlaServerServiceImpl.h"
#include "olad/PluginLoader.h"
#include "olad/Universe.h"
#include "olad/UniverseStore.h"

using google::protobuf::Closure;
using google::protobuf::NewCallback;
using ola::DmxBuffer;
using ola::OlaServerServiceImpl;
using ola::Universe;
using ola::UniverseStore;
using ola::rpc::SimpleRpcController;
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
      m_uid(OPEN_LIGHTING_ESTA_CODE, 0) {
    }
    void testGetDmx();
    void testRegisterForDmx();
    void testUpdateDmxData();
    void testSetUniverseName();
    void testSetMergeMode();

  private:
    ola::rdm::UID m_uid;
    void CallGetDmx(OlaServerServiceImpl *impl,
                    int universe_id,
                    class GetDmxCheck &check);
    void CallRegisterForDmx(OlaServerServiceImpl *impl,
                            int universe_id,
                            ola::proto::RegisterAction action,
                            class RegisterForDmxCheck &check);
    void CallUpdateDmxData(OlaServerServiceImpl *impl,
                           int universe_id,
                           const DmxBuffer &data,
                           class UpdateDmxDataCheck &check);
    void CallSetUniverseName(OlaServerServiceImpl *impl,
                             int universe_id,
                             const string &name,
                             class SetUniverseNameCheck &check);
    void CallSetMergeMode(OlaServerServiceImpl *impl,
                       int universe_id,
                       ola::proto::MergeMode merge_mode,
                       class SetMergeModeCheck &check);
};

CPPUNIT_TEST_SUITE_REGISTRATION(OlaServerServiceImplTest);

static const uint8_t SAMPLE_DMX_DATA[] = {1, 2, 3, 4, 5};

/*
 * The GetDmx Checks
 */
class GetDmxCheck {
  public:
    virtual ~GetDmxCheck() {}
    virtual void Check(SimpleRpcController *controller,
                       ola::proto::DmxData *reply) = 0;
};


/*
 * Assert that the data is all 0
 */
class GetDmxNoDataCheck: public GetDmxCheck {
  public:
    void Check(SimpleRpcController *controller,
               ola::proto::DmxData *reply) {
      DmxBuffer empty_buffer;
      CPPUNIT_ASSERT(!controller->Failed());
      CPPUNIT_ASSERT(empty_buffer == DmxBuffer(reply->data()));
    }
};


/*
 * Assert that the data matches the test data.
 */
class GetDmxValidDataCheck: public GetDmxCheck {
  public:
    void Check(SimpleRpcController *controller,
               ola::proto::DmxData *reply) {
      CPPUNIT_ASSERT(!controller->Failed());
      CPPUNIT_ASSERT(DmxBuffer(SAMPLE_DMX_DATA, sizeof(SAMPLE_DMX_DATA)) ==
                     DmxBuffer(reply->data()));
    }
};


/*
 * RegisterForDmxChecks
 */
class RegisterForDmxCheck {
  public:
    virtual ~RegisterForDmxCheck() {}
    virtual void Check(SimpleRpcController *controller,
                       ola::proto::Ack *reply) = 0;
};


/*
 * UpdateDmxDataCheck
 */
class UpdateDmxDataCheck {
  public:
    virtual ~UpdateDmxDataCheck() {}
    virtual void Check(SimpleRpcController *controller,
                       ola::proto::Ack *reply) = 0;
};


/*
 * SetUniverseNameCheck
 */
class SetUniverseNameCheck {
  public:
    virtual ~SetUniverseNameCheck() {}
    virtual void Check(SimpleRpcController *controller,
                       ola::proto::Ack *reply) = 0;
};


/*
 * SetMergeModeCheck
 */
class SetMergeModeCheck {
  public:
    virtual ~SetMergeModeCheck() {}
    virtual void Check(SimpleRpcController *controller,
                       ola::proto::Ack *reply) = 0;
};


/*
 * Assert that we got a missing universe error
 */
template<typename parent, typename reply>
class GenericMissingUniverseCheck: public parent {
  public:
    void Check(SimpleRpcController *controller,
               reply *r) {
      CPPUNIT_ASSERT(controller->Failed());
      CPPUNIT_ASSERT_EQUAL(string("Universe doesn't exist"),
                           controller->ErrorText());
      (void) r;
    }
};


/*
 * Assert that we got an ack
 */
template<typename parent>
class GenericAckCheck: public parent {
  public:
    void Check(SimpleRpcController *controller,
               ola::proto::Ack *r) {
      CPPUNIT_ASSERT(!controller->Failed());
      (void) r;
    }
};


/*
 * Check that the GetDmx method works
 */
void OlaServerServiceImplTest::testGetDmx() {
  UniverseStore store(NULL, NULL);
  OlaServerServiceImpl impl(&store,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            m_uid);

  GenericMissingUniverseCheck<GetDmxCheck, ola::proto::DmxData>
    missing_universe_check;
  GetDmxNoDataCheck empty_data_check;
  GetDmxValidDataCheck valid_data_check;

  // test a universe that doesn't exist
  unsigned int universe_id = 0;
  CallGetDmx(&impl, universe_id, missing_universe_check);

  // test a new universe
  Universe *universe = store.GetUniverseOrCreate(universe_id);
  CPPUNIT_ASSERT(universe);
  CallGetDmx(&impl, universe_id, empty_data_check);

  // Set the universe data
  DmxBuffer buffer(SAMPLE_DMX_DATA, sizeof(SAMPLE_DMX_DATA));
  universe->SetDMX(buffer);
  CallGetDmx(&impl, universe_id, valid_data_check);

  // remove the universe and try again
  store.AddUniverseGarbageCollection(universe);
  store.GarbageCollectUniverses();
  CallGetDmx(&impl, universe_id, missing_universe_check);
}


/*
 * Call the GetDmx method
 * @param impl the OlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param check the GetDmxCheck class to use for the callback check
 */
void OlaServerServiceImplTest::CallGetDmx(OlaServerServiceImpl *impl,
                                          int universe_id,
                                          GetDmxCheck &check) {
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::UniverseRequest *request = new ola::proto::UniverseRequest();
  ola::proto::DmxData *response = new ola::proto::DmxData();
  Closure *closure = NewCallback(
      &check,
      &GetDmxCheck::Check,
      controller,
      response);

  request->set_universe(universe_id);
  impl->GetDmx(controller, request, response, closure);
  delete controller;
  delete request;
  delete response;
}


/*
 * Check the RegisterForDmx method works
 */
void OlaServerServiceImplTest::testRegisterForDmx() {
  UniverseStore store(NULL, NULL);
  OlaServerServiceImpl impl(&store,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            m_uid);

  // Register for a universe that doesn't exist
  unsigned int universe_id = 0;
  unsigned int second_universe_id = 99;
  GenericAckCheck<RegisterForDmxCheck> ack_check;
  CallRegisterForDmx(&impl, universe_id, ola::proto::REGISTER, ack_check);

  // The universe should exist now and the client should be bound
  Universe *universe = store.GetUniverse(universe_id);
  CPPUNIT_ASSERT(universe);
  CPPUNIT_ASSERT(universe->ContainsSinkClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, universe->SinkClientCount());

  // Try to register again
  CallRegisterForDmx(&impl, universe_id, ola::proto::REGISTER, ack_check);
  CPPUNIT_ASSERT(universe->ContainsSinkClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, universe->SinkClientCount());

  // Register a second universe
  CallRegisterForDmx(&impl, second_universe_id, ola::proto::REGISTER,
                     ack_check);
  Universe *second_universe = store.GetUniverse(universe_id);
  CPPUNIT_ASSERT(second_universe->ContainsSinkClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, second_universe->SinkClientCount());

  // Unregister the first universe
  CallRegisterForDmx(&impl, universe_id, ola::proto::UNREGISTER, ack_check);
  CPPUNIT_ASSERT(!universe->ContainsSinkClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SinkClientCount());

  // Unregister the second universe
  CallRegisterForDmx(&impl, second_universe_id, ola::proto::UNREGISTER,
                     ack_check);
  CPPUNIT_ASSERT(!second_universe->ContainsSinkClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, second_universe->SinkClientCount());

  // Unregister again
  CallRegisterForDmx(&impl, universe_id, ola::proto::UNREGISTER, ack_check);
  CPPUNIT_ASSERT(!universe->ContainsSinkClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->SinkClientCount());
}


/*
 * Call the RegisterForDmx method
 * @param impl the OlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param action the action to use REGISTER or UNREGISTER
 * @param check the RegisterForDmxCheck to use for the callback check
 */
void OlaServerServiceImplTest::CallRegisterForDmx(
    OlaServerServiceImpl *impl,
    int universe_id,
    ola::proto::RegisterAction action,
    RegisterForDmxCheck &check) {
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::RegisterDmxRequest *request = (
      new ola::proto::RegisterDmxRequest());
  ola::proto::Ack *response = new ola::proto::Ack();
  Closure *closure = NewCallback(
      &check,
      &RegisterForDmxCheck::Check,
      controller,
      response);

  request->set_universe(universe_id);
  request->set_action(action);
  impl->RegisterForDmx(controller, request, response, closure);
  delete controller;
  delete request;
  delete response;
}


/*
 * Check the UpdateDmxData method works
 */
void OlaServerServiceImplTest::testUpdateDmxData() {
  UniverseStore store(NULL, NULL);
  ola::TimeStamp time1, time2;
  ola::Client client(NULL);
  ola::Client client2(NULL);
  OlaServerServiceImpl impl(&store,
                            NULL,
                            NULL,
                            &client,
                            NULL,
                            NULL,
                            NULL,
                            &time1,
                            m_uid);
  OlaServerServiceImpl impl2(&store,
                             NULL,
                             NULL,
                             &client2,
                             NULL,
                             NULL,
                             NULL,
                             &time2,
                             m_uid);

  GenericMissingUniverseCheck<UpdateDmxDataCheck, ola::proto::Ack>
    missing_universe_check;
  GenericAckCheck<UpdateDmxDataCheck> ack_check;
  unsigned int universe_id = 0;
  DmxBuffer dmx_data("this is a test");
  DmxBuffer dmx_data2("different data hmm");

  // Update a universe that doesn't exist
  ola::Clock::CurrentTime(&time1);
  CallUpdateDmxData(&impl, universe_id, dmx_data, missing_universe_check);
  Universe *universe = store.GetUniverse(universe_id);
  CPPUNIT_ASSERT(!universe);

  // Update a universe that exists
  ola::Clock::CurrentTime(&time1);
  universe = store.GetUniverseOrCreate(universe_id);
  CallUpdateDmxData(&impl, universe_id, dmx_data, ack_check);
  CPPUNIT_ASSERT(dmx_data == universe->GetDMX());

  // Update a second client with an older timestamp
  // make sure we're in ltp mode
  CPPUNIT_ASSERT_EQUAL(universe->MergeMode(), Universe::MERGE_LTP);
  time2 = time1 - ola::TimeInterval(1000000);
  CallUpdateDmxData(&impl2, universe_id, dmx_data2, ack_check);
  CPPUNIT_ASSERT_EQUAL(dmx_data.Size(), universe->GetDMX().Size());
  // Should continue to hold the old data
  CPPUNIT_ASSERT(dmx_data == universe->GetDMX());

  // Now send a new update
  ola::Clock::CurrentTime(&time2);
  CallUpdateDmxData(&impl2, universe_id, dmx_data2, ack_check);
  CPPUNIT_ASSERT(dmx_data2 == universe->GetDMX());
}


/*
 * Call the UpdateDmxDataCheck method
 * @param impl the OlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param data the DmxBuffer to use as data
 * @param check the SetUniverseNameCheck to use for the callback check
 */
void OlaServerServiceImplTest::CallUpdateDmxData(
    OlaServerServiceImpl *impl,
    int universe_id,
    const DmxBuffer &data,
    UpdateDmxDataCheck &check) {
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::DmxData *request = new
    ola::proto::DmxData();
  ola::proto::Ack *response = new ola::proto::Ack();
  Closure *closure = NewCallback(
      &check,
      &UpdateDmxDataCheck::Check,
      controller,
      response);

  request->set_universe(universe_id);
  request->set_data(data.Get());
  impl->UpdateDmxData(controller, request, response, closure);
  delete controller;
  delete request;
  delete response;
}


/*
 * Check the SetUniverseName method works
 */
void OlaServerServiceImplTest::testSetUniverseName() {
  UniverseStore store(NULL, NULL);
  OlaServerServiceImpl impl(&store,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            m_uid);

  unsigned int universe_id = 0;
  string universe_name = "test 1";
  string universe_name2 = "test 1-2";

  GenericAckCheck<SetUniverseNameCheck> ack_check;
  GenericMissingUniverseCheck<SetUniverseNameCheck, ola::proto::Ack>
    missing_universe_check;

  // Check we get an error for a missing universe
  CallSetUniverseName(&impl, universe_id, universe_name,
                      missing_universe_check);
  Universe *universe = store.GetUniverse(universe_id);
  CPPUNIT_ASSERT(!universe);

  // Check SetUniverseName works on an existing univserse
  universe = store.GetUniverseOrCreate(universe_id);
  CallSetUniverseName(&impl, universe_id, universe_name, ack_check);
  CPPUNIT_ASSERT_EQUAL(universe_name, universe->Name());

  // Run it again with a new name
  CallSetUniverseName(&impl, universe_id, universe_name2, ack_check);
  CPPUNIT_ASSERT_EQUAL(universe_name2, universe->Name());
}


/*
 * Call the SetUniverseName method
 * @param impl the OlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param name the name to use
 * @param check the SetUniverseNameCheck to use for the callback check
 */
void OlaServerServiceImplTest::CallSetUniverseName(
    OlaServerServiceImpl *impl,
    int universe_id,
    const string &name,
    SetUniverseNameCheck &check) {
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::UniverseNameRequest *request = new
    ola::proto::UniverseNameRequest();
  ola::proto::Ack *response = new ola::proto::Ack();
  Closure *closure = NewCallback(
      &check,
      &SetUniverseNameCheck::Check,
      controller,
      response);

  request->set_universe(universe_id);
  request->set_name(name);
  impl->SetUniverseName(controller, request, response, closure);

  delete controller;
  delete request;
  delete response;
}


/*
 * Check the SetMergeMode method works
 */
void OlaServerServiceImplTest::testSetMergeMode() {
  UniverseStore store(NULL, NULL);
  OlaServerServiceImpl impl(&store,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            m_uid);

  unsigned int universe_id = 0;

  GenericAckCheck<SetMergeModeCheck> ack_check;
  GenericMissingUniverseCheck<SetMergeModeCheck, ola::proto::Ack>
    missing_universe_check;

  // Check we get an error for a missing universe
  CallSetMergeMode(&impl, universe_id, ola::proto::HTP, missing_universe_check);
  Universe *universe = store.GetUniverse(universe_id);
  CPPUNIT_ASSERT(!universe);

  // Check SetUniverseName works
  universe = store.GetUniverseOrCreate(universe_id);
  CallSetMergeMode(&impl, universe_id, ola::proto::HTP, ack_check);
  CPPUNIT_ASSERT(Universe::MERGE_HTP == universe->MergeMode());

  // Run it again
  CallSetMergeMode(&impl, universe_id, ola::proto::LTP, ack_check);
  CPPUNIT_ASSERT(Universe::MERGE_LTP == universe->MergeMode());
}


/*
 * Call the SetMergeMode method
 * @param impl the OlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param mode the merge_mode to use
 * @param check the SetMergeModeCheck to use for the callback check
 */
void OlaServerServiceImplTest::CallSetMergeMode(
    OlaServerServiceImpl *impl,
    int universe_id,
    ola::proto::MergeMode merge_mode,
    SetMergeModeCheck &check) {
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::MergeModeRequest *request = new
    ola::proto::MergeModeRequest();
  ola::proto::Ack *response = new ola::proto::Ack();
  Closure *closure = NewCallback(
      &check,
      &SetMergeModeCheck::Check,
      controller,
      response);

  request->set_universe(universe_id);
  request->set_merge_mode(merge_mode);
  impl->SetMergeMode(controller, request, response, closure);

  delete controller;
  delete request;
  delete response;
}
