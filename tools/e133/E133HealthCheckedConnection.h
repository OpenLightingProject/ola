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
 * E133HealthCheckedConnection.h
 * Copyright (C) 2012 Simon Newton
 *
 * This class detects unhealthy TCP connections. A TCP connection is defined as
 * healthy if it can pass data in both directions. Both ends must implement the
 * same health checking logic (and agree on heartbeat intervals) for this to
 * work correctly.
 *
 * Even though this is called a E1.33 Health Checked Connection, it doesn't
 * actually rely on E1.33 at all. You can use it with any ACN based protocol
 * since it just sends PDUs with a ROOT_VECTOR_NULL as heartbeat messages.
 */

#ifndef TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_
#define TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/e133/MessageBuilder.h>
#include <ola/io/MemoryBlockPool.h>
#include <ola/network/HealthCheckedConnection.h>
#include <ola/thread/SchedulingExecutorInterface.h>

#include <memory>

#include "tools/e133/MessageQueue.h"

/**
 * An E1.33 health checked connection.
 */
class E133HealthCheckedConnection
    : public ola::network::HealthCheckedConnection {
 public:
    E133HealthCheckedConnection(
        ola::e133::MessageBuilder *message_builder,
        MessageQueue *message_queue,
        ola::SingleUseCallback0<void> *on_timeout,
        ola::thread::SchedulingExecutorInterface *scheduler,
        const ola::TimeInterval heartbeat_interval =
          ola::TimeInterval(E133_TCP_HEARTBEAT_INTERVAL, 0));

    void SendHeartbeat();
    void HeartbeatTimeout();

 private:
    ola::e133::MessageBuilder *m_message_builder;
    MessageQueue *m_message_queue;
    std::auto_ptr<ola::SingleUseCallback0<void> > m_on_timeout;
    ola::thread::SchedulingExecutorInterface *m_executor;

    // The default interval in seconds for sending heartbeat messages.
    static const unsigned int E133_TCP_HEARTBEAT_INTERVAL = 5;
};
#endif  // TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_
