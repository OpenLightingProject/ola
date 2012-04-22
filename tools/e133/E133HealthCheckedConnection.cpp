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
 * E133HealthCheckedConnection.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>

#include "plugins/e131/e131/RootSender.h"
#include "tools/e133/E133HealthCheckedConnection.h"

/**
 * Create a new E1.33 Health Checked Connection.
 * @param transport the transport to send heartbeats on
 * @param sender the RootSender to use when sending heartbeats
 * @param on_timeout the callback to run when the heartbeats don't arrive
 * @param scheduler A SchedulerInterface used to control the timers
 * @param vector the vector to use in the RootPDUs
 * @param heartbeat_interval the TimeInterval between heartbeats
 */
E133HealthCheckedConnection::E133HealthCheckedConnection(
  ola::plugin::e131::OutgoingStreamTransport *transport,
  ola::plugin::e131::RootSender *sender,
  ola::SingleUseCallback0<void> *on_timeout,
  ola::thread::SchedulerInterface *scheduler,
  unsigned int vector,
  const ola::TimeInterval heartbeat_interval)
    : HealthCheckedConnection(scheduler, heartbeat_interval),
      m_vector(vector),
      m_transport(transport),
      m_sender(sender),
      m_on_timeout(on_timeout) {
}


/**
 * Send a E1.33 heartbeat
 */
void E133HealthCheckedConnection::SendHeartbeat() {
  OLA_INFO << "Sending heartbeat";
  if (!m_sender->SendEmpty(m_vector, m_transport))
    OLA_WARN << "Failed to send heartbeat";
}


/**
 * Called if the connection is declared dead
 */
void E133HealthCheckedConnection::HeartbeatTimeout() {
  OLA_INFO << "TCP connection heartbeat timeout";
  if (m_on_timeout)
    m_on_timeout->Run();
}
