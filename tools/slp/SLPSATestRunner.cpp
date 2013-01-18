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
using std::auto_ptr;
using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;


XIDAllocator TestCase::xid_allocator(0);
map<string, TestCaseCreator> _test_creators;


bool TestCase::VerifySLPHeader(const uint8_t *data, unsigned int length,
                               ola::slp::slp_function_id_t function_id,
                               uint16_t flags, xid_t xid) {
  uint8_t actual_function_id = SLPPacketParser::DetermineFunctionID(data,
      length);
  if (actual_function_id != function_id) {
    OLA_INFO << "Function ID " << static_cast<int>(actual_function_id)
             << ", doesn't match expected: " << static_cast<int>(function_id);
    return false;
  }

  MemoryBuffer buffer(data, length);
  BigEndianInputStream stream(&buffer);
  SLPPacket slp_packet;
  if (!SLPPacketParser::ExtractHeader(&stream, &slp_packet, Name())) {
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

  OLA_INFO << "Starting to run " << m_tests.size() << " tests against "
           << m_target;
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

  TestCase *test = *m_running_test;
  if (test->expected_result() != TestCase::RESULT_DATA) {
    OLA_WARN << test->Name() << " received an un-expected reply";
    test->SetState(TestCase::FAILED);
    RunNextTest();
    return;
  }

  OLA_INFO << "Got " << packet_size << " bytes from target";
  test->SetState(test->VerifyReply(packet, packet_size));
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
  OLA_INFO << "Running " << test->Name();
  test->BuildPacket(&m_output_stream);

  // figure out where to send the packet
  IPV4SocketAddress target;
  switch (test->destination()) {
    case TestCase::DESTINATION_UNDEFINED:
      OLA_WARN << test->Name() << " did not specify a target";
      test->SetState(TestCase::BROKEN);
      RunNextTest();
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
    RunNextTest();
    return;
  }

  OLA_INFO << "Sending " << m_output_queue.Size() << " bytes to " << target;
  m_socket.SendTo(&m_output_queue, target);

  m_ss.RegisterSingleTimeout(
      m_timeout_in_ms,
      NewSingleCallback(this, &TestRunner::TestTimeout));
}


void TestRunner::TestTimeout() {
  TestCase *test = *m_running_test;
  if (test->expected_result() != TestCase::RESULT_TIMEOUT) {
    OLA_WARN << test->Name() << " expected a reply but didn't get one";
    test->SetState(TestCase::FAILED);
  } else {
    OLA_INFO << test->Name() << " passed";
    test->SetState(TestCase::PASSED);
  }
  RunNextTest();
}
