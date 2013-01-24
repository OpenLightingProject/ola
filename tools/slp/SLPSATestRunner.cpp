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
 * SLPSATestRunner.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/io/BigEndianStream.h>
#include <ola/io/IOQueue.h>
#include <ola/io/MemoryBuffer.h>
#include <ola/io/SelectServer.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/Socket.h>
#include <ola/stl/STLUtils.h>

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/SLPSATestRunner.h"
#include "tools/slp/XIDAllocator.h"

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::io::BigEndianOutputStream;
using ola::io::BigEndianInputStream;
using ola::io::IOQueue;
using ola::io::SelectServer;
using ola::io::MemoryBuffer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::Interface;
using ola::network::InterfacePicker;
using ola::network::UDPSocket;
using ola::slp::SLPPacketParser;
using ola::slp::XIDAllocator;
using ola::slp::SLPPacket;
using ola::slp::xid_t;
using ola::thread::INVALID_TIMEOUT;
using std::auto_ptr;
using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;


XIDAllocator TestCase::xid_allocator(0);
map<string, TestCaseCreator> _test_creators;


/**
 * Called when network data arrives. This allows the test to check if the
 * response is valid.
 */
TestCase::TestState TestCase::VerifyReceivedData(const uint8_t *data,
                                                 unsigned int length) {
  if (expected_result() == RESULT_DATA) {
    OLA_INFO << "Got " << length << " bytes from target";
    if (!CheckSLPHeader(data, length, m_function_id, 0, GetXID()))
      return FAILED;
    return VerifyReply(data, length);
  } else if (expected_result() == RESULT_ERROR) {
    return CheckSLPErrorResponse(data, length, m_function_id, 0, GetXID(),
                                 m_error_code);
  } else {
    OLA_WARN << Name() << " received an un-expected reply";
    return FAILED;
  }
}


/**
 * Check that the SLP message starts with the expected function-id.
 */
bool TestCase::CheckFunctionID(const uint8_t *data, unsigned int length,
                               slp_function_id_t function_id) {
  uint8_t actual_function_id = SLPPacketParser::DetermineFunctionID(data,
      length);
  if (actual_function_id != function_id) {
    OLA_INFO << "Function ID " << static_cast<int>(actual_function_id)
             << ", doesn't match expected: " << static_cast<int>(function_id);
    return false;
  }
  return true;
}


/**
 * Check that this message starts with the expected SLP header.
 */
bool TestCase::CheckSLPHeader(const uint8_t *data, unsigned int length,
                              slp_function_id_t function_id,
                              uint16_t flags, xid_t xid) {
  if (!CheckFunctionID(data, length, function_id))
    return false;

  MemoryBuffer buffer(data, length);
  BigEndianInputStream stream(&buffer);
  return VerifySLPHeader(&stream, flags, xid);
}


/**
 * Check that the response data is a valid SLP error message
 */
TestCase::TestState TestCase::CheckSLPErrorResponse(
    const uint8_t *data, unsigned int length,
    slp_function_id_t function_id, uint16_t flags, xid_t xid,
    uint16_t error_code) {
  if (!CheckFunctionID(data, length, function_id))
    return FAILED;

  MemoryBuffer buffer(data, length);
  BigEndianInputStream stream(&buffer);

  if (!VerifySLPHeader(&stream, flags, xid))
    return FAILED;

  // read error from stream here
  uint16_t actual_error_code;
  if (!(stream >> actual_error_code)) {
    OLA_INFO << "Packet too small to contain error code";
    return FAILED;
  }

  if (error_code != actual_error_code) {
    OLA_INFO << "Error code doesn't match expected";
    return FAILED;
  }
  return PASSED;
}


/**
 * Given an input stream, verify that the SLP header matches the expected
 * values. This does not check the SLP fucntion id.
 * @returns true if the header matches, false otherwise.
 */
bool TestCase::VerifySLPHeader(BigEndianInputStream *stream,
                               uint16_t flags, xid_t xid) {
  SLPPacket slp_packet;
  if (!SLPPacketParser::ExtractHeader(stream, &slp_packet, Name())) {
    return false;
  }

  if (slp_packet.xid != xid) {
    OLA_INFO << "XID mismatch, expected " << static_cast<int>(xid) << ", got "
             << static_cast<int>(slp_packet.xid);
    return false;
  }

  if (slp_packet.flags != flags) {
    OLA_INFO << "Flags mismatch, expected " << static_cast<int>(flags)
             << ", got " << static_cast<int>(slp_packet.flags);
    return false;
  }

  if (slp_packet.language != "en") {
    OLA_INFO << "Language mismatch, expected 'en', got "
             << slp_packet.language;
    return false;
  }
  return true;
}



TestCaseCreatorMap &GetTestCreatorMap() {
  static TestCaseCreatorMap test_creators;
  return test_creators;
}

void CreateTests(vector<TestCase*> *test_cases) {
  TestCaseCreatorMap &test_creators = GetTestCreatorMap();
  TestCaseCreatorMap::iterator iter = test_creators.begin();
  for (; iter != test_creators.end(); ++iter) {
    TestCase *test = (*(iter->second))();
    test->SetName(iter->first);
    test_cases->push_back(test);
  }
}

void CreateTestsMatchingnames(const vector<string> &test_names,
                              vector<TestCase*> *test_cases) {
  TestCaseCreatorMap &test_creators = GetTestCreatorMap();
  vector<string>::const_iterator iter = test_names.begin();
  for (; iter != test_names.end(); ++iter) {
    TestCaseCreatorMap::iterator test_iter = test_creators.find(*iter);
    if (test_iter != test_creators.end()) {
      TestCase *test = (*(test_iter->second))();
      test->SetName(test_iter->first);
      test_cases->push_back(test);
    }
  }
}

void GetTestnames(vector<string> *test_names) {
  TestCaseCreatorMap &test_creators = GetTestCreatorMap();
  TestCaseCreatorMap::iterator iter = test_creators.begin();
  for (; iter != test_creators.end(); ++iter)
    test_names->push_back(iter->first);
}

TestRunner::TestRunner(unsigned int timeout,
                       const vector<string> &test_names,
                       const IPV4SocketAddress &target)
    : m_timeout_in_ms(timeout),
      m_target(target),
      m_multicast_endpoint(
          IPV4Address::FromStringOrDie("239.255.255.253"), target.Port()),
      m_timeout_id(INVALID_TIMEOUT),
      m_output_stream(&m_output_queue) {
  if (test_names.empty()) {
    CreateTests(&m_tests);
  } else {
    CreateTestsMatchingnames(test_names, &m_tests);
  }
  m_test_to_run = m_tests.begin();
  m_running_test = m_tests.end();
}


TestRunner::~TestRunner() {
  ola::STLDeleteValues(m_tests);
}

void TestRunner::Run() {
  if (!m_socket.Init())
    return;

  IPV4SocketAddress local_endpoint(IPV4Address::WildCard(), 0);
  if (!m_socket.Bind(local_endpoint))
    return;

  auto_ptr<InterfacePicker> picker(InterfacePicker::NewPicker());
  Interface interface;
  if (!picker->ChooseInterface(&interface, "")) {
    OLA_INFO << "Failed to pick interface";
    return;
  }

  m_socket.SetMulticastInterface(interface.ip_address);
  m_socket.SetOnData(NewCallback(this, &TestRunner::ReceiveData));
  m_ss.AddReadDescriptor(&m_socket);

  cout << "Starting to run " << m_tests.size() << " tests against "
           << m_target << endl;
  m_ss.Execute(NewSingleCallback(this, &TestRunner::RunNextTest));
  m_ss.Run();

  unsigned int failed = 0;
  unsigned int passed = 0;
  unsigned int not_run = 0;
  unsigned int broken = 0;
  vector<TestCase*>::const_iterator iter = m_tests.begin();
  for (; iter != m_tests.end(); ++iter) {
    switch ((*iter)->test_state()) {
      case TestCase::BROKEN:
        broken++;
        break;
      case TestCase::NOT_RUN:
        not_run++;
        break;
      case TestCase::FAILED:
        failed++;
        break;
      case TestCase::PASSED:
        passed++;
        break;
    }
  }

  cout << "-----------------------------------------------------" << endl;
  unsigned int total = passed + failed + not_run + broken;
  cout << passed << "/" << total << " Passed, " << failed << "/"
       << total << " Failed, " << broken << " Broken" << endl;

  m_ss.RemoveReadDescriptor(&m_socket);
  m_socket.Close();
}


void TestRunner::ReceiveData() {
  ssize_t packet_size = 1500;
  uint8_t packet[packet_size];
  ola::network::IPV4Address source_ip;
  uint16_t port;

  if (!m_socket.RecvFrom(reinterpret_cast<uint8_t*>(&packet),
                         &packet_size, source_ip, port))
    return;

  IPV4SocketAddress source(source_ip, port);
  if (source != m_target) {
    OLA_INFO << "Ignoring message from " << source;
    return;
  }

  if (m_timeout_id != INVALID_TIMEOUT) {
    m_ss.RemoveTimeout(m_timeout_id);
    m_timeout_id = INVALID_TIMEOUT;
  }

  TestCase *test = *m_running_test;
  test->SetState(test->VerifyReceivedData(packet, packet_size));
  CompleteTest();
}

void TestRunner::CompleteTest() {
  TestCase *test = *m_running_test;
  cout << test->Name() << ": ";
  switch (test->test_state()) {
    case TestCase::BROKEN:
      cout << "\x1b[33mBroken";
      break;
    case TestCase::NOT_RUN:
      cout << "\x1b[31mNot Run";
      break;
    case TestCase::FAILED:
      cout << "\x1b[31mFailed";
      break;
    case TestCase::PASSED:
      cout << "\x1b[32mPassed";
      break;
  }
  cout << "\x1b[0m" << endl;
  RunNextTest();
}

void TestRunner::RunNextTest() {
  m_running_test = m_test_to_run++;
  if (m_running_test == m_tests.end()) {
    m_ss.Terminate();
    return;
  }

  TestCase *test = *m_running_test;
  test->SetDestinationIP(m_target.Host());
  cout << "Running " << test->Name() << endl;
  test->BuildPacket(&m_output_stream);

  // figure out where to send the packet
  IPV4SocketAddress target;
  switch (test->destination()) {
    case TestCase::DESTINATION_UNDEFINED:
      OLA_WARN << test->Name() << " did not specify a target";
      test->SetState(TestCase::BROKEN);
      CompleteTest();
      return;
    case TestCase::UNICAST:
      target = m_target;
      break;
    case TestCase::MULTICAST:
      target = m_multicast_endpoint;
      break;
  }

  if (test->expected_result() == TestCase::RESULT_UNDEFINED) {
    OLA_WARN << test->Name() << " did not specify an expected result";
    test->SetState(TestCase::BROKEN);
    CompleteTest();
    return;
  }

  OLA_INFO << "Sending " << m_output_queue.Size() << " bytes to " << target;
  m_socket.SendTo(&m_output_queue, target);

  m_timeout_id = m_ss.RegisterSingleTimeout(
      m_timeout_in_ms,
      NewSingleCallback(this, &TestRunner::TestTimeout));
}


void TestRunner::TestTimeout() {
  TestCase *test = *m_running_test;
  if (test->expected_result() != TestCase::RESULT_TIMEOUT) {
    OLA_WARN << test->Name() << " expected a reply but didn't get one";
    test->SetState(TestCase::FAILED);
  } else {
    test->SetState(TestCase::PASSED);
  }
  CompleteTest();
}
