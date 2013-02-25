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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SLPSATestRunner.h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef TOOLS_SLP_SLPSATESTRUNNER_H_
#define TOOLS_SLP_SLPSATESTRUNNER_H_

#include <ola/Callback.h>
#include <ola/io/IOQueue.h>
#include <ola/io/BigEndianStream.h>
#include <ola/io/SelectServer.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Socket.h>
#include <ola/stl/STLUtils.h>

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "tools/slp/XIDAllocator.h"
#include "tools/slp/SLPPacketConstants.h"

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::io::BigEndianInputStream;
using ola::io::BigEndianOutputStream;
using ola::io::IOQueue;
using ola::io::SelectServer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::UDPSocket;
using ola::slp::XIDAllocator;
using ola::slp::slp_function_id_t;
using ola::slp::xid_t;
using std::map;
using std::set;
using std::string;
using std::vector;


class TestCase {
  public:
    typedef enum {
      DESTINATION_UNDEFINED,
      UNICAST,
      MULTICAST,
    } Destination;

    typedef enum {
      RESULT_UNDEFINED,
      RESULT_TIMEOUT,
      RESULT_DATA,
      RESULT_ERROR,
    } ExpectedResult;

    typedef enum {
      NOT_RUN,
      BROKEN,
      PASSED,
      FAILED,
    } TestState;

    typedef enum {
      UNDEFINED,
      SRVRQST,
      ERROR_HANDLING,
    } TestCategory;

    TestCase()
        : m_target(DESTINATION_UNDEFINED),
          m_expected_result(RESULT_UNDEFINED),
          m_test_state(NOT_RUN),
          m_xid_assigned(false),
          m_xid(0) {
    }

    virtual ~TestCase() {}

    // The sub class overrides this method to build the packet
    virtual void BuildPacket(BigEndianOutputStream *output) = 0;

    TestState VerifyReceivedData(const uint8_t *data, unsigned int length);

    void SetName(const string &name) { m_name = name; }
    string Name() const { return m_name; }

    ExpectedResult expected_result() const { return m_expected_result; }

    Destination destination() const { return m_target; }

    IPV4Address GetDestinationIP() const { return m_destination_ip; }
    void SetDestinationIP(const IPV4Address &destination) {
      m_destination_ip = destination;
    }

    TestState test_state() { return m_test_state; }
    void SetState(TestState state) { m_test_state = state; }

    // Returns the xid allocated for this test
    xid_t GetXID() {
      if (!m_xid_assigned) {
        m_xid_assigned = true;
        m_xid = xid_allocator.Next();
      }
      return m_xid;
    }

    virtual bool CheckLangInResponse(const string &lang) const {
      if (lang != ola::slp::EN_LANGUAGE_TAG) {
        OLA_INFO << "Language mismatch, expected '" << ola::slp::EN_LANGUAGE_TAG
                 << "', got " << lang;
        return false;
      }
      return true;
    }

  protected:
    // tests can use this.
    set<IPV4Address> pr_list;

    void SetDestination(Destination target) { m_target = target; }

    // If ExpectResponse() was called and the SLP header of the received packet
    // matched, this will be called. This allows each test to check the
    // contents of the SLP response.
    virtual TestState VerifyReply(const uint8_t*, unsigned int) {
      return BROKEN;
    }

    void ExpectTimeout() {
      SetExpectedResult(RESULT_TIMEOUT, "ExpectTimeout");
    }

    void ExpectError(slp_function_id_t function_id, uint16_t error_code) {
      SetExpectedResult(RESULT_ERROR, "ExpectError");
      m_function_id = function_id;
      m_error_code = error_code;
    }

    void ExpectResponse(slp_function_id_t function_id) {
      SetExpectedResult(RESULT_DATA, "ExpectResponse");
      m_function_id = function_id;
    }

  private:
    string m_name;
    Destination m_target;
    IPV4Address m_destination_ip;
    ExpectedResult m_expected_result;
    TestState m_test_state;
    bool m_xid_assigned;
    xid_t m_xid;
    slp_function_id_t m_function_id;
    uint16_t m_error_code;

    void SetExpectedResult(ExpectedResult result, const string& method) {
      if (m_expected_result != RESULT_UNDEFINED) {
        OLA_WARN << Name() << " " << method << " overriding previous value";
      }
      m_expected_result = result;
    }

    bool CheckFunctionID(const uint8_t *data, unsigned int length,
                         slp_function_id_t function_id);

    bool CheckSLPHeader(const uint8_t *data, unsigned int length,
                        slp_function_id_t function_id,
                        uint16_t flags, xid_t xid);

    TestState CheckSLPErrorResponse(const uint8_t *data, unsigned int length,
                                    slp_function_id_t function_id,
                                    uint16_t flags, xid_t xid,
                                    uint16_t error_code);

    bool VerifySLPHeader(BigEndianInputStream *stream,
                         uint16_t flags, xid_t xid);

    static XIDAllocator xid_allocator;
};

/**
 * The procedure for registering tests.
 *
 *  Each test should subclass TestCase, and provide a BuildPacket() method.
 *  Then call REGISTER_TEST(CLASS_NAME) to register the test:
 *
 *  class FooTest: public TestCase {
 *    void BuildPacket(BigEndianOutputStream *) {
 *      // something
 *    }
 *  };
 *  REGISTER_TEST(FooTest);
 */

typedef TestCase* (*TestCaseCreator)();
typedef map<string, TestCaseCreator> TestCaseCreatorMap;

// Returns a reference to the map that holds the test creation functions.
TestCaseCreatorMap &GetTestCreatorMap();


// Instatiate all known tests.
void CreateTests(vector<TestCase*> *test_cases);
// Instatiate tests which match the test names given in test_names.
void CreateTestsMatchingnames(const vector<string> &test_names,
                              vector<TestCase*> *test_cases);
// Return a list of all registered tests.
void GetTestnames(vector<string> *test_names);


/**
 * An object which registers a test creator on construction.
 */
class TestRegisterer {
  public:
    TestRegisterer(const string &test_name, TestCaseCreator creator) {
      GetTestCreatorMap()[test_name] = creator;
    }
};


#define REGISTER_TEST(test_class) \
  TestCase *New##test_class() { \
    return new test_class(); \
  } \
  static TestRegisterer test_registerer_##test_class( \
      #test_class, &New##test_class);


/**
 * The TestRunner is the class which executes all the tests.
 */
class TestRunner {
  public:
    TestRunner(unsigned int timeout,
               const vector<string> &test_names,
               const IPV4SocketAddress &target);
    ~TestRunner();

    void Run();

  private:
    SelectServer m_ss;
    UDPSocket m_socket;
    const unsigned int m_timeout_in_ms;
    const IPV4SocketAddress m_target, m_multicast_endpoint;
    ola::io::timeout_id m_timeout_id;
    IOQueue m_output_queue;
    BigEndianOutputStream m_output_stream;
    vector<TestCase*> m_tests;
    // points to the next test to run
    vector<TestCase*>::iterator m_test_to_run;
    // points to the running test
    vector<TestCase*>::iterator m_running_test;

    void CompleteTest();
    void RunNextTest();
    void TestTimeout();
    void ReceiveData();
};
#endif  // TOOLS_SLP_SLPSATESTRUNNER_H_
