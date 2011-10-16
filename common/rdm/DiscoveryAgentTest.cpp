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
 * DiscoveryAgentTest.cpp
 * Test fixture for the DiscoveryAgent class
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <vector>

#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/rdm/DiscoveryAgent.h"
#include "common/rdm/DiscoveryAgentTestHelper.h"

using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::rdm::DiscoveryAgent;
using std::vector;


typedef std::vector<class MockResponderInterface*> ResponderList;


class DiscoveryAgentTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DiscoveryAgentTest);
  CPPUNIT_TEST(testNoReponders);
  CPPUNIT_TEST(testSingleResponder);
  CPPUNIT_TEST(testMultipleResponders);
  CPPUNIT_TEST(testObnoxiousResponder);
  CPPUNIT_TEST(testBipolarResponder);
  CPPUNIT_TEST(testNonMutingResponder);
  CPPUNIT_TEST(testProxy);
  CPPUNIT_TEST_SUITE_END();

  public:
    DiscoveryAgentTest()
        : CppUnit::TestFixture(),
          m_callback_run(false) {
    }
    void testNoReponders();
    void testSingleResponder();
    void testMultipleResponders();
    void testObnoxiousResponder();
    void testBipolarResponder();
    void testNonMutingResponder();
    void testProxy();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

  private:
    bool m_callback_run;

    void DiscoverySuccessful(const UIDSet *expected,
                             bool successful,
                             const UIDSet &received);
    void DiscoveryFailed(const UIDSet *expected,
                         bool successful,
                         const UIDSet &received);
    void PopulateResponderListFromUIDs(const UIDSet &uids,
                                       ResponderList *responders);
};


CPPUNIT_TEST_SUITE_REGISTRATION(DiscoveryAgentTest);


/**
 * Called when discovery completes
 */
void DiscoveryAgentTest::DiscoverySuccessful(const UIDSet *expected,
                                             bool successful,
                                             const UIDSet &received) {
  OLA_INFO << "in discovery callback, size is " << received.Size() <<
      ", state: " << successful;
  CPPUNIT_ASSERT(successful);
  CPPUNIT_ASSERT_EQUAL(*expected, received);
  m_callback_run = true;
}

/**
 * Called when discovery completes and fails for some reason.
 */
void DiscoveryAgentTest::DiscoveryFailed(const UIDSet *expected,
                                         bool successful,
                                         const UIDSet &received) {
  OLA_INFO << "in discovery callback, size is " << received.Size() <<
      ", state: " << successful;
  CPPUNIT_ASSERT(!successful);
  CPPUNIT_ASSERT_EQUAL(*expected, received);
  m_callback_run = true;
}


/**
 * Build a vector of MockResponder objects with the given uids.
 */
void DiscoveryAgentTest::PopulateResponderListFromUIDs(
    const UIDSet &uids,
    ResponderList *responders) {
  UIDSet::Iterator iter = uids.Begin();
  for (; iter != uids.End(); iter++)
    responders->push_back(new MockResponder(*iter));
}


/*
 * Test the case where we have no responders.
 */
void DiscoveryAgentTest::testNoReponders() {
  UIDSet uids;
  ResponderList responders;
  PopulateResponderListFromUIDs(uids, &responders);
  MockDiscoveryTarget target(responders);
  DiscoveryAgent agent(&target);
  OLA_INFO << "starting discovery with no responders";

  agent.StartFullDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoverySuccessful,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);

  // now try incremental
  OLA_INFO << "starting incremental discovery with no responders";
  agent.StartIncrementalDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoverySuccessful,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);
}


/**
 * Test single responder
 */
void DiscoveryAgentTest::testSingleResponder() {
  UIDSet uids;
  ResponderList responders;
  uids.AddUID(UID(1, 10));
  PopulateResponderListFromUIDs(uids, &responders);
  MockDiscoveryTarget target(responders);

  DiscoveryAgent agent(&target);
  OLA_INFO << "starting discovery with one responder";
  agent.StartFullDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoverySuccessful,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);
  m_callback_run = false;

  // now try incremental
  OLA_INFO << "starting incremental discovery with one responder";
  agent.StartIncrementalDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoverySuccessful,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);
}


/**
 * Test multiple responders
 */
void DiscoveryAgentTest::testMultipleResponders() {
  UIDSet uids;
  ResponderList responders;
  UID uid_to_remove(0x7a70, 0x00002001);
  uids.AddUID(uid_to_remove);
  uids.AddUID(UID(0x7a70, 0x00002002));
  uids.AddUID(UID(0x7a77, 0x00002002));
  PopulateResponderListFromUIDs(uids, &responders);
  MockDiscoveryTarget target(responders);

  DiscoveryAgent agent(&target);
  OLA_INFO << "starting discovery with two responder";
  agent.StartFullDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoverySuccessful,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);
  m_callback_run = false;

  // now try incremental, adding one uid and removing another
  UID uid_to_add(0x8080, 0x00103456);
  uids.RemoveUID(uid_to_remove);
  uids.AddUID(uid_to_add);
  // update the responder list
  target.RemoveResponder(uid_to_remove);
  target.AddResponder(new MockResponder(uid_to_add));

  OLA_INFO << "starting incremental discovery with modified responder list";
  agent.StartIncrementalDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoverySuccessful,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);
}


/**
 * Test a responder that continues to responder when muted.
 */
void DiscoveryAgentTest::testObnoxiousResponder() {
  UIDSet uids;
  ResponderList responders;
  uids.AddUID(UID(0x7a70, 0x00002002));
  PopulateResponderListFromUIDs(uids, &responders);
  // add the ObnoxiousResponder
  UID obnoxious_uid = UID(0x7a77, 0x00002002);
  UID obnoxious_uid2 = UID(0x7a77, 0x00003030);
  uids.AddUID(obnoxious_uid);
  uids.AddUID(obnoxious_uid2);
  responders.push_back(new ObnoxiousResponder(obnoxious_uid));
  responders.push_back(new ObnoxiousResponder(obnoxious_uid2));
  MockDiscoveryTarget target(responders);

  DiscoveryAgent agent(&target);
  OLA_INFO << "starting discovery with two responder";
  agent.StartFullDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoveryFailed,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);
  m_callback_run = false;

  // now try incremental, adding one uid and removing another
  OLA_INFO << "starting incremental discovery with modified responder list";
  agent.StartIncrementalDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoveryFailed,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);
}


/**
 * Test a responder that can't make up it's mind about it's UID
 */
void DiscoveryAgentTest::testBipolarResponder() {
  UIDSet uids;
  ResponderList responders;
  uids.AddUID(UID(0x7a70, 0x00002002));
  PopulateResponderListFromUIDs(uids, &responders);
  // add the BiPolarResponders
  UID bipolar_uid = UID(0x7a77, 0x00002002);
  UID bipolar_uid2 = UID(0x7a77, 0x00003030);
  responders.push_back(new BiPolarResponder(bipolar_uid));
  responders.push_back(new BiPolarResponder(bipolar_uid2));
  MockDiscoveryTarget target(responders);

  DiscoveryAgent agent(&target);
  OLA_INFO << "starting discovery with BiPolarResponder responder";
  agent.StartFullDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoveryFailed,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);
  m_callback_run = false;

  // now try incremental, adding one uid and removing another
  OLA_INFO << "starting incremental discovery with modified responder list";
  agent.StartIncrementalDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoveryFailed,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);
}


/**
 * Test a responder that doesn't respond to a mute message.
 */
void DiscoveryAgentTest::testNonMutingResponder() {
  UIDSet uids;
  ResponderList responders;
  uids.AddUID(UID(0x7a70, 0x00002002));
  PopulateResponderListFromUIDs(uids, &responders);
  // add the NonMutingResponders
  UID non_muting_uid = UID(0x7a77, 0x00002002);
  UID non_muting_uid2 = UID(0x7a77, 0x00003030);
  responders.push_back(new NonMutingResponder(non_muting_uid));
  responders.push_back(new NonMutingResponder(non_muting_uid2));
  MockDiscoveryTarget target(responders);

  DiscoveryAgent agent(&target);
  OLA_INFO << "starting discovery with NonMutingResponder responder";
  agent.StartFullDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoveryFailed,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);
  m_callback_run = false;
}


/**
 * Test a proxy.
 */
void DiscoveryAgentTest::testProxy() {
  UIDSet uids;
  ResponderList proxied_responders, responders;
  uids.AddUID(UID(0x7a70, 0x00002002));
  uids.AddUID(UID(0x8080, 0x00001234));
  PopulateResponderListFromUIDs(uids, &proxied_responders);
  // add the Proxy
  UID proxy_uid = UID(0x1010, 0x00002002);
  uids.AddUID(proxy_uid);
  responders.push_back(new ProxyResponder(proxy_uid, proxied_responders));
  MockDiscoveryTarget target(responders);

  DiscoveryAgent agent(&target);
  OLA_INFO << "starting discovery with Proxy responder";
  agent.StartFullDiscovery(
      ola::NewSingleCallback(this,
                             &DiscoveryAgentTest::DiscoverySuccessful,
                             static_cast<const UIDSet*>(&uids)));
  CPPUNIT_ASSERT(m_callback_run);
  m_callback_run = false;
}
