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
  const ola::TimeInterval timeout_interval)
    : m_scheduler(scheduler),
      m_heartbeat_interval(timeout_interval),
      m_send_timeout_id(ola::thread::INVALID_TIMEOUT),
      m_receive_timeout_id(ola::thread::INVALID_TIMEOUT) {
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

  // pretend we just send a heartbeat
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
  TimeInterval timeout_interval(2.5 * m_heartbeat_interval.AsInt());
  m_receive_timeout_id = m_scheduler->RegisterSingleTimeout(
    timeout_interval,
    NewSingleCallback(this, &HealthCheckedConnection::HeartbeatTimeout));
}
}  // network
}  // ola
