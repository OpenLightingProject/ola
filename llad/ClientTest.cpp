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

#include "Client.h"
#include "common/protocol/Lla.pb.h"

using namespace lla;
using namespace std;

static unsigned int TEST_UNIVERSE = 1;
static uint8_t TEST_DMX_DATA[] = "this is some test data";

class ClientTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ClientTest);
  CPPUNIT_TEST(testSendDMX);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSendDMX();
};


/*
 * Mock out the ClientStub for testing
 */
class MockClientStub: public lla::proto::LlaClientService_Stub {
  public:
      MockClientStub(): lla::proto::LlaClientService_Stub(NULL) {}
      void UpdateDmxData(::google::protobuf::RpcController* controller,
                         const ::lla::proto::DmxData* request,
                         ::lla::proto::Ack* response,
                         ::google::protobuf::Closure* done);
};

CPPUNIT_TEST_SUITE_REGISTRATION(ClientTest);


void MockClientStub::UpdateDmxData(
    ::google::protobuf::RpcController* controller,
    const ::lla::proto::DmxData* request,
    ::lla::proto::Ack* response,
    ::google::protobuf::Closure* done) {

  CPPUNIT_ASSERT(controller);
  CPPUNIT_ASSERT(!controller->Failed());
  CPPUNIT_ASSERT_EQUAL(TEST_UNIVERSE, (unsigned int) request->universe());
  DmxBuffer buffer(TEST_DMX_DATA, sizeof(TEST_DMX_DATA));
  CPPUNIT_ASSERT(buffer == DmxBuffer(request->data()));
  done->Run();
}


/*
 * Check that the SendDMX method works correctly.
 */
void ClientTest::testSendDMX() {

  // test we survive a null pointer
  DmxBuffer buffer(TEST_DMX_DATA, sizeof(TEST_DMX_DATA));
  Client *client = new Client(NULL);
  client->SendDMX(TEST_UNIVERSE, buffer);
  delete client;

  MockClientStub client_stub;
  client = new Client(&client_stub);
  client->SendDMX(TEST_UNIVERSE, buffer);
  delete client;
}
