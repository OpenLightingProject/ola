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
 * ClientTest.cpp
 * Test fixture for the Client class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "common/protocol/Ola.pb.h"
#include "common/protocol/OlaService.pb.h"
#include "common/rpc/RpcController.h"
#include "common/rpc/RpcService.h"
#include "ola/Clock.h"
#include "ola/DmxBuffer.h"
#include "ola/testing/TestUtils.h"
#include "olad/Client.h"
#include "olad/DmxSource.h"


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

  private:
    ola::Clock m_clock;
};


CPPUNIT_TEST_SUITE_REGISTRATION(ClientTest);


/*
 * Mock out the ClientStub for testing
 */
class MockClientStub: public ola::proto::OlaClientService_Stub {
  public:
    MockClientStub(): ola::proto::OlaClientService_Stub(NULL) {}

    void UpdateDmxData(ola::rpc::RpcController *controller,
                       const ola::proto::DmxData *request,
                       ola::proto::Ack *response,
                       ola::rpc::RpcService::CompletionCallback *done);
};



void MockClientStub::UpdateDmxData(
    ola::rpc::RpcController* controller,
    const ola::proto::DmxData *request,
    ola::proto::Ack *response,
    ola::rpc::RpcService::CompletionCallback *done) {
  OLA_ASSERT(controller);
  OLA_ASSERT_FALSE(controller->Failed());
  OLA_ASSERT_EQ(TEST_UNIVERSE, (unsigned int) request->universe());
  OLA_ASSERT(TEST_DATA == request->data());
  done->Run();
  (void) response;
}


/*
 * Check that the SendDMX method works correctly.
 */
void ClientTest::testSendDMX() {
  // check we survive a null pointer
  const DmxBuffer buffer(TEST_DATA);
  uint8_t priority = 100;
  Client client(NULL);
  OLA_ASSERT(NULL == client.Stub());
  client.SendDMX(TEST_UNIVERSE, priority, buffer);

  // check the stub is called correctly
  MockClientStub client_stub;
  Client client2(&client_stub);
  OLA_ASSERT(&client_stub == client2.Stub());
  client2.SendDMX(TEST_UNIVERSE, priority, buffer);
}


/*
 * Check that the DMX get/set works correctly.
 */
void ClientTest::testGetSetDMX() {
  DmxBuffer buffer(TEST_DATA);
  const DmxBuffer empty;
  Client client(NULL);

  ola::TimeStamp timestamp;
  m_clock.CurrentTime(&timestamp);
  ola::DmxSource source(buffer, timestamp, 100);

  // check get/set works
  client.DMXRecieved(TEST_UNIVERSE, source);
  const ola::DmxSource &source2 = client.SourceData(TEST_UNIVERSE);
  OLA_ASSERT(source2.IsSet());
  OLA_ASSERT(source2.Data() == buffer);
  OLA_ASSERT_EQ(timestamp, source2.Timestamp());
  OLA_ASSERT_EQ((uint8_t) 100, source2.Priority());

  // check update works
  ola::DmxBuffer old_data(buffer);
  buffer.Set(TEST_DATA2);
  OLA_ASSERT(source2.Data() == old_data);
  OLA_ASSERT_EQ(timestamp, source2.Timestamp());
  OLA_ASSERT_EQ((uint8_t) 100, source2.Priority());

  source.UpdateData(buffer, timestamp, 120);
  client.DMXRecieved(TEST_UNIVERSE, source);
  const ola::DmxSource source3 = client.SourceData(TEST_UNIVERSE);
  OLA_ASSERT(source3.IsSet());
  OLA_ASSERT(buffer == source3.Data());
  OLA_ASSERT_EQ(timestamp, source3.Timestamp());
  OLA_ASSERT_EQ((uint8_t) 120, source3.Priority());

  // check fetching an unknown universe results in an empty buffer
  const ola::DmxSource source4 = client.SourceData(TEST_UNIVERSE2);
  OLA_ASSERT_FALSE(source4.IsSet());
  OLA_ASSERT(empty == source4.Data());
}
