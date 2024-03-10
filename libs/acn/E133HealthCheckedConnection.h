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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * E133HealthCheckedConnection.h
 * Copyright (C) 2012 Simon Newton
 *
 * This class detects unhealthy TCP connections. A TCP connection is defined as
 * healthy if it can pass data in both directions. Both ends must implement the
 * same health checking logic (and agree on heartbeat intervals) for this to
 * work correctly.
 *
 * This is a E1.33 Health Checked Connection as it sends E1.33 Broker NULL PDUs
 * using VECTOR_BROKER_NULL, but it also accepts any ACN root layer PDUs as a
 * positive indication the connection is healthy.
 *
 * You could use it with any ACN based protocol by subclassing it and sending
 * heartbeat messages of ROOT_VECTOR_NULL via SendHeartbeat instead.
 */

#ifndef TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_
#define TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/e133/MessageBuilder.h>
#include <ola/io/MemoryBlockPool.h>
#include <ola/io/NonBlockingSender.h>
#include <ola/network/HealthCheckedConnection.h>
#include <ola/thread/SchedulingExecutorInterface.h>

#include <memory>


/**
 * An E1.33 health checked connection.
 */
class E133HealthCheckedConnection
    : public ola::network::HealthCheckedConnection {
 public:
    E133HealthCheckedConnection(
        ola::e133::MessageBuilder *message_builder,
        ola::io::NonBlockingSender *message_queue,
        ola::SingleUseCallback0<void> *on_timeout,
        ola::thread::SchedulingExecutorInterface *scheduler,
        const ola::TimeInterval heartbeat_interval =
          ola::TimeInterval(E133_TCP_HEARTBEAT_INTERVAL, 0),
        const ola::TimeInterval timeout_interval =
          ola::TimeInterval(E133_HEARTBEAT_TIMEOUT, 0));

    void SendHeartbeat();
    void HeartbeatTimeout();

 private:
    ola::e133::MessageBuilder *m_message_builder;
    ola::io::NonBlockingSender *m_message_queue;
    std::auto_ptr<ola::SingleUseCallback0<void> > m_on_timeout;
    ola::thread::SchedulingExecutorInterface *m_executor;

    // The default interval in seconds for sending heartbeat messages.
    static const unsigned int E133_TCP_HEARTBEAT_INTERVAL = 15;
    // The default interval in seconds before timing out.
    static const unsigned int E133_HEARTBEAT_TIMEOUT = 45;
};
#endif  // TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_
