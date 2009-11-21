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
 * ClientTest.cpp
 * Test fixture for the Client class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/DmxBuffer.h"
#include "olad//Client.h"
#include "common/protocol/Ola.pb.h"


static unsigned int TEST_UNIVERSE = 1;
static unsigned int TEST_UNIVERSE2 = 2;
static const char TEST_DATA[] = "this is some test data";
static const char TEST_DATA2[] = "another set of test data";

using ola::Client;
using ola::DmxBuffer;
using std::string;


class ClientTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ClientTest);
  CPPUNIT_TEST(testSendDMX);
  CPPUNIT_TEST(testGetSetDMX);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSendDMX();
    void testGetSetDMX();
};


CPPUNIT_TEST_SUITE_REGISTRATION(ClientTest);


/*
 * Mock out the ClientStub for testing
 */
class MockClientStub: public ola::proto::OlaClientService_Stub {
  public:
      MockClientStub(): ola::proto::OlaClientService_Stub(NULL) {}
      void UpdateDmxData(::google::protobuf::RpcController* controller,
                         const ::ola::proto::DmxData* request,
                         ::ola::proto::Ack* response,
                         ::google::protobuf::Closure* done);
};



void MockClientStub::UpdateDmxData(
    ::google::protobuf::RpcController* controller,
    const ::ola::proto::DmxData* request,
    ::ola::proto::Ack* response,
    ::google::protobuf::Closure* done) {

  CPPUNIT_ASSERT(controller);
  CPPUNIT_ASSERT(!controller->Failed());
  CPPUNIT_ASSERT_EQUAL(TEST_UNIVERSE, (unsigned int) request->universe());
  CPPUNIT_ASSERT(TEST_DATA == request->data());
  done->Run();
}


/*
 * Check that the SendDMX method works correctly.
 */
void ClientTest::testSendDMX() {
  // check we survive a null pointer
  const DmxBuffer buffer(TEST_DATA);
  Client client(NULL);
  CPPUNIT_ASSERT(NULL == client.Stub());
  client.SendDMX(TEST_UNIVERSE, buffer);

  // check the stub is called correctly
  MockClientStub client_stub;
  Client client2(&client_stub);
  CPPUNIT_ASSERT(&client_stub == client2.Stub());
  client2.SendDMX(TEST_UNIVERSE, buffer);
}


/*
 * Check that the DMX get/set works correctly.
 */
void ClientTest::testGetSetDMX() {
  DmxBuffer buffer(TEST_DATA);
  const DmxBuffer empty;
  Client client(NULL);

  // check get/set works
  client.DMXRecieved(TEST_UNIVERSE, buffer);
  DmxBuffer result = client.GetDMX(TEST_UNIVERSE);
  CPPUNIT_ASSERT(buffer == result);

  // check update works
  buffer.Set(TEST_DATA2);
  client.DMXRecieved(TEST_UNIVERSE, buffer);
  result = client.GetDMX(TEST_UNIVERSE);
  CPPUNIT_ASSERT(buffer == result);
  CPPUNIT_ASSERT_EQUAL(string(TEST_DATA2), result.Get());

  // check fetching an unknown universe results in an empty buffer
  result = client.GetDMX(TEST_UNIVERSE2);
  CPPUNIT_ASSERT(empty == result);
}
