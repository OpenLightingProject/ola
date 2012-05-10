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
 * HealthCheckedConnectionTest.cpp
 * Test fixture for the HealthCheckedConnection class.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/HealthCheckedConnection.h"
#include "ola/network/Socket.h"

using ola::MockClock;
using ola::NewCallback;
using ola::NewSingleCallback;
using ola::TimeInterval;
using ola::network::HealthCheckedConnection;
using ola::io::LoopbackDescriptor;
using ola::io::SelectServer;


class MockHealthCheckedConnection: public HealthCheckedConnection {
  public:
    struct Options {
      uint8_t end_after;  // terminate after this many heartbeats
      uint8_t send_every;  // don't send every N heartbeats
      bool validate_heartbeat;  // check the heartbeat is what we expect
      bool abort_on_failure;  // fail if the channel goes down
    };

    static void InitOptions(Options *options) {
      options->end_after = 8;
      options->send_every = 0;
      options->validate_heartbeat = false;
      options->abort_on_failure = true;
    }

    MockHealthCheckedConnection(ola::io::ConnectedDescriptor *descriptor,
                                SelectServer *scheduler,
                                const ola::TimeInterval timeout_interval,
                                const Options &options,
                                MockClock *clock)
        : HealthCheckedConnection(scheduler, timeout_interval),
          m_descriptor(descriptor),
          m_ss(scheduler),
          m_options(options),
          m_next_heartbeat(0),
          m_expected_heartbeat(0),
          m_channel_ok(true),
          m_clock(clock) {
    }

    void SendHeartbeat() {
      if (m_options.send_every == 0 ||
          m_next_heartbeat % m_options.send_every == 0) {
        m_descriptor->Send(&m_next_heartbeat, sizeof(m_next_heartbeat));
      }
      m_clock->AdvanceTime(0, 180000);
      m_next_heartbeat++;
    }

    void HeartbeatTimeout() {
      if (m_options.abort_on_failure)
        CPPUNIT_FAIL("Channel went down");
      m_channel_ok = false;
      m_ss->Terminate();
    }

    void ReadData() {
      uint8_t data;
      unsigned int data_read;
      m_descriptor->Receive(&data, sizeof(data), data_read);
      if (m_options.validate_heartbeat)
        CPPUNIT_ASSERT_EQUAL(m_expected_heartbeat++, data);
      HeartbeatReceived();

      if (data >= m_options.end_after)
        m_ss->Terminate();
    }

    bool ChannelOk() const { return m_channel_ok; }

  private:
    ola::io::ConnectedDescriptor *m_descriptor;
    SelectServer *m_ss;
    Options m_options;
    uint8_t m_next_heartbeat;
    uint8_t m_expected_heartbeat;
    bool m_channel_ok;
    MockClock *m_clock;
};


class HealthCheckedConnectionTest: public CppUnit::TestFixture {
  public:
    HealthCheckedConnectionTest()
        : CppUnit::TestFixture(),
          m_ss(NULL, &m_clock),
          heartbeat_interval(0, 200000) {
    }

  CPPUNIT_TEST_SUITE(HealthCheckedConnectionTest);
  CPPUNIT_TEST(testSimpleChannel);
  CPPUNIT_TEST(testChannelWithPacketLoss);
  CPPUNIT_TEST(testChannelWithHeavyPacketLoss);
  CPPUNIT_TEST(testPauseAndResume);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown() {}
    void testSimpleChannel();
    void testChannelWithPacketLoss();
    void testChannelWithHeavyPacketLoss();
    void testPauseAndResume();

    void PauseReading(MockHealthCheckedConnection *connection) {
      connection->PauseTimer();
      m_ss.RemoveReadDescriptor(&socket);
    }

    void ResumeReading(MockHealthCheckedConnection *connection) {
      connection->ResumeTimer();
      m_ss.AddReadDescriptor(&socket);
    }

  private:
    MockClock m_clock;
    SelectServer m_ss;
    LoopbackDescriptor socket;
    TimeInterval heartbeat_interval;
    MockHealthCheckedConnection::Options options;
};


CPPUNIT_TEST_SUITE_REGISTRATION(HealthCheckedConnectionTest);


void HealthCheckedConnectionTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  socket.Init();
  MockHealthCheckedConnection::InitOptions(&options);
}


/*
 * Check that the channel stays up when all heartbeats are recieved.
 */
void HealthCheckedConnectionTest::testSimpleChannel() {
  options.validate_heartbeat = true;
  MockHealthCheckedConnection connection(&socket,
                                         &m_ss,
                                         heartbeat_interval,
                                         options,
                                         &m_clock);

  socket.SetOnData(
      NewCallback(&connection, &MockHealthCheckedConnection::ReadData));
  connection.Setup();
  m_ss.AddReadDescriptor(&socket);
  connection.Setup();

  m_ss.Run();
  CPPUNIT_ASSERT(connection.ChannelOk());
}


/**
 * Check the channel works when every 2nd heartbeat is lost
 */
void HealthCheckedConnectionTest::testChannelWithPacketLoss() {
  options.send_every = 2;
  MockHealthCheckedConnection connection(&socket,
                                         &m_ss,
                                         heartbeat_interval,
                                         options,
                                         &m_clock);

  socket.SetOnData(
      NewCallback(&connection, &MockHealthCheckedConnection::ReadData));
  connection.Setup();
  m_ss.AddReadDescriptor(&socket);
  connection.Setup();

  m_ss.Run();
  CPPUNIT_ASSERT(connection.ChannelOk());
}


/**
 * Check the channel works when every 2nd heartbeat is lost
 */
void HealthCheckedConnectionTest::testChannelWithHeavyPacketLoss() {
  options.send_every = 3;
  options.abort_on_failure = false;
  MockHealthCheckedConnection connection(&socket,
                                         &m_ss,
                                         heartbeat_interval,
                                         options,
                                         &m_clock);

  socket.SetOnData(
      NewCallback(&connection, &MockHealthCheckedConnection::ReadData));
  connection.Setup();
  m_ss.AddReadDescriptor(&socket);
  connection.Setup();

  m_ss.Run();
  CPPUNIT_ASSERT(!connection.ChannelOk());
}


/**
 * Check pausing doesn't mark the channel as bad.
 */
void HealthCheckedConnectionTest::testPauseAndResume() {
  MockHealthCheckedConnection connection(&socket,
                                         &m_ss,
                                         heartbeat_interval,
                                         options,
                                         &m_clock);
  socket.SetOnData(
      NewCallback(&connection, &MockHealthCheckedConnection::ReadData));
  connection.Setup();
  m_ss.AddReadDescriptor(&socket);
  connection.Setup();

  m_ss.RegisterSingleTimeout(
      TimeInterval(1, 0),
      NewSingleCallback(this,
                        &HealthCheckedConnectionTest::PauseReading,
                        &connection));
  m_ss.RegisterSingleTimeout(
      TimeInterval(3, 0),
      NewSingleCallback(this,
                        &HealthCheckedConnectionTest::ResumeReading,
                        &connection));

  m_ss.Run();
  CPPUNIT_ASSERT(connection.ChannelOk());
}
