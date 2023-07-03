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
 * E133HealthCheckedConnection.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/io/IOStack.h>
#include <ola/io/NonBlockingSender.h>

#include "libs/acn/RootSender.h"
#include "tools/e133/E133HealthCheckedConnection.h"

using ola::io::IOStack;

/**
 * Create a new E1.33 Health Checked Connection.
 * @param message_builder the MessageBuilder to use to create packets.
 * @param message_queue the NonBlockingSender to use to send packets.
 * @param on_timeout the callback to run when the heartbeats don't arrive
 * @param scheduler A SchedulerInterface used to control the timers
 * @param heartbeat_interval the TimeInterval between heartbeats
 */
E133HealthCheckedConnection::E133HealthCheckedConnection(
  ola::e133::MessageBuilder *message_builder,
  ola::io::NonBlockingSender *message_queue,
  ola::SingleUseCallback0<void> *on_timeout,
  ola::thread::SchedulingExecutorInterface *scheduler,
  const ola::TimeInterval heartbeat_interval,
  const ola::TimeInterval timeout_interval)
    : HealthCheckedConnection(scheduler, heartbeat_interval, timeout_interval),
      m_message_builder(message_builder),
      m_message_queue(message_queue),
      m_on_timeout(on_timeout),
      m_executor(scheduler) {
}


/**
 * Send a E1.33 heartbeat
 */
void E133HealthCheckedConnection::SendHeartbeat() {
  OLA_DEBUG << "Sending heartbeat";
  IOStack packet(m_message_builder->pool());
  m_message_builder->BuildBrokerNullTCPPacket(&packet);
  m_message_queue->SendMessage(&packet);
}


/**
 * Called if the connection is declared dead
 */
void E133HealthCheckedConnection::HeartbeatTimeout() {
  OLA_INFO << "TCP connection heartbeat timeout";
  if (m_on_timeout.get()) {
    m_executor->Execute(m_on_timeout.release());
  }
}
