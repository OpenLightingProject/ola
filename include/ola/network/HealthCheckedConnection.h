/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * HealthCheckedConnection.h
 * Copyright (C) 2012 Simon Newton
 *
 * This class adds health checking to a connection, which ensures that the
 * connection is able to transfer data in a timely manner. The implementation
 * is pretty simple: we define a heart beat interval I, which *must* be the
 * same at both ends of the connection. Every I seconds, both ends send a
 * heart beat message and if either end doesn't receive a heart beat in
 * 2.5 * I, the connection is deemed dead, and the connection is closed.
 *
 * This class provides the basic health check mechanism, the sub class is left
 * to define the format of the heartbeat message.
 *
 * To use this health checked channel, subclass HealthCheckedConnection, and
 * provide the SendHeartbeat() and HeartbeatTimeout methods.
 *
 * There are some additional features:
 *  - Some receivers may want to stop reading from a connection under some
 *  circumstances (e.g. flow control). Before this happens, call PauseTimer()
 *  to pause the rx timer, otherwise the channel will be marked unhealthy. Once
 *  reading is resumed called ResumeTimer();
 *  - Some protocols may want to piggyback heartbeats on other messages, or
 *  even count any message as a heartbeat. When such a message is received, be
 *  sure to call HeartbeatReceived() which will update the timer.
 */

#ifndef INCLUDE_OLA_NETWORK_HEALTHCHECKEDCONNECTION_H_
#define INCLUDE_OLA_NETWORK_HEALTHCHECKEDCONNECTION_H_

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/base/Macro.h>
#include <ola/thread/SchedulerInterface.h>

namespace ola {
namespace network {

/**
 * An class that provides health checking for a connection.
 * The subclasses implement the SendHeartbeat and HeartbeatTimeout methods.
 */
class HealthCheckedConnection {
 public:
    HealthCheckedConnection(ola::thread::SchedulerInterface *scheduler,
                            const ola::TimeInterval timeout_interval);
    virtual ~HealthCheckedConnection();

    /**
     * Setup the health checked channel
     */
    bool Setup();

    // Sending methods
    //-----------------------------

    /*
     * Subclasses implement this to send a health check
     */
    virtual void SendHeartbeat() = 0;

    /**
     * Call this when a heartbeat is piggybacked on another message. This
     * prevents sending heartbeats unless necessary.
     */
    void HeartbeatSent();


    // Receiving methods
    //-----------------------------

    // Call this method every time a valid health check is received.
    void HeartbeatReceived();

    /*
     * This pauses the timer which checks for heartbeats. Call this if you stop
     * reading from the socket for any reason.
     */
    void PauseTimer();

    /**
     * Resume the health check timer. Call this when reading is resumed.
     */
    void ResumeTimer();

 protected:
    /**
     * This is called when we don't receive a health check within the interval.
     */
    virtual void HeartbeatTimeout() = 0;

 private:
    ola::thread::SchedulerInterface *m_scheduler;
    ola::TimeInterval m_heartbeat_interval;
    ola::thread::timeout_id m_send_timeout_id;
    ola::thread::timeout_id m_receive_timeout_id;

    bool SendNextHeartbeat();
    void UpdateReceiveTimer();
    void InternalHeartbeatTimeout();

    DISALLOW_COPY_AND_ASSIGN(HealthCheckedConnection);
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_HEALTHCHECKEDCONNECTION_H__
