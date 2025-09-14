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
 * HealthCheckedConnection.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include "ola/Logging.h"
#include "ola/network/HealthCheckedConnection.h"
#include "ola/thread/SchedulerInterface.h"

namespace ola {
namespace network {

HealthCheckedConnection::HealthCheckedConnection(
  ola::thread::SchedulerInterface *scheduler,
  const ola::TimeInterval heartbeat_interval,
  const ola::TimeInterval timeout_interval)
    : m_scheduler(scheduler),
      m_heartbeat_interval(heartbeat_interval),
      m_timeout_interval(timeout_interval),
      m_send_timeout_id(ola::thread::INVALID_TIMEOUT),
      m_receive_timeout_id(ola::thread::INVALID_TIMEOUT) {
}


HealthCheckedConnection::HealthCheckedConnection(
  ola::thread::SchedulerInterface *scheduler,
  const ola::TimeInterval heartbeat_interval)
    : HealthCheckedConnection(scheduler,
                              heartbeat_interval,
                              ola::TimeInterval(static_cast<int>(
                                  2.5 * heartbeat_interval.AsInt()))) {
}

HealthCheckedConnection::~HealthCheckedConnection() {
  if (m_send_timeout_id != ola::thread::INVALID_TIMEOUT)
    m_scheduler->RemoveTimeout(m_send_timeout_id);
  if (m_receive_timeout_id != ola::thread::INVALID_TIMEOUT)
    m_scheduler->RemoveTimeout(m_receive_timeout_id);
}


bool HealthCheckedConnection::Setup() {
  // setup the RX timeout
  ResumeTimer();

  // send a heartbeat now and setup the TX timer
  SendHeartbeat();
  HeartbeatSent();
  return true;
}


/**
 * Reset the send timer
 */
void HealthCheckedConnection::HeartbeatSent() {
  if (m_send_timeout_id != ola::thread::INVALID_TIMEOUT)
    m_scheduler->RemoveTimeout(m_send_timeout_id);
  m_send_timeout_id = m_scheduler->RegisterRepeatingTimeout(
    m_heartbeat_interval,
    NewCallback(this, &HealthCheckedConnection::SendNextHeartbeat));
}


/**
 * Reset the RX timer
 */
void HealthCheckedConnection::HeartbeatReceived() {
  m_scheduler->RemoveTimeout(m_receive_timeout_id);
  UpdateReceiveTimer();
}


/**
 * Pause the receive timer
 */
void HealthCheckedConnection::PauseTimer() {
  if (m_receive_timeout_id != ola::thread::INVALID_TIMEOUT) {
    m_scheduler->RemoveTimeout(m_receive_timeout_id);
    m_receive_timeout_id = ola::thread::INVALID_TIMEOUT;
  }
}


/**
 * Resume the receive timer
 */
void HealthCheckedConnection::ResumeTimer() {
  if (m_receive_timeout_id == ola::thread::INVALID_TIMEOUT)
    UpdateReceiveTimer();
}


bool HealthCheckedConnection::SendNextHeartbeat() {
  SendHeartbeat();
  return true;
}


void HealthCheckedConnection::UpdateReceiveTimer() {
  m_receive_timeout_id = m_scheduler->RegisterSingleTimeout(
    m_timeout_interval,
    NewSingleCallback(
      this, &HealthCheckedConnection::InternalHeartbeatTimeout));
}

void HealthCheckedConnection::InternalHeartbeatTimeout() {
  m_receive_timeout_id = ola::thread::INVALID_TIMEOUT;
  HeartbeatTimeout();
}
}  // namespace network
}  // namespace ola
