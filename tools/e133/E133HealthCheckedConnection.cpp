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

#include "plugins/e131/e131/RDMInflator.h"
#include "plugins/e131/e131/RDMPDU.h"
#include "tools/e133/E133Endpoint.h"
#include "tools/e133/E133HealthCheckedConnection.h"
#include "tools/e133/E133StreamSender.h"

/**
 * Create a new E1.33 Health Checked Connection
 */
E133HealthCheckedConnection::E133HealthCheckedConnection(
  E133StreamSender *sender,
  ola::SingleUseCallback0<void> *on_timeout,
  ola::thread::SchedulerInterface *scheduler,
  const ola::TimeInterval timeout_interval)
    : HealthCheckedConnection(scheduler, timeout_interval),
      m_on_timeout(on_timeout),
      m_sender(sender) {
}


/**
 * Send a E1.33 heartbeat
 */
void E133HealthCheckedConnection::SendHeartbeat() {
  OLA_INFO << "Sending heartbeat";

  // heartbeats are just empty RDM messages
  const ola::plugin::e131::RDMPDU pdu(NULL);
  bool result = m_sender->Send(
    ola::plugin::e131::RDMInflator::RDM_VECTOR,
    ROOT_E133_ENDPOINT,
    pdu);
  if (!result)
    OLA_WARN << "Failed to send E1.33 response";
}


/**
 * Called if the connection is declared dead
 */
void E133HealthCheckedConnection::HeartbeatTimeout() {
  OLA_INFO << "E1.33 TCP connection is dead";
  if (m_on_timeout)
    m_on_timeout->Run();
}
