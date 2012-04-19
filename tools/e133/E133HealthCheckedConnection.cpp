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

#include "plugins/e131/e131/E133Sender.h"
#include "plugins/e131/e131/RDMPDU.h"
#include "plugins/e131/e131/TCPTransport.h"
#include "tools/e133/E133HealthCheckedConnection.h"


E133HealthCheckedConnection::E133HealthCheckedConnection(
  E133Sender *sender,
  ola::SingleUseCallback0<void> *on_timeout,
  ola::network::ConnectedDescriptor *descriptor,
  ola::thread::SchedulerInterface *scheduler,
  const ola::TimeInterval timeout_interval)
    : HealthCheckedConnection(descriptor, scheduler, timeout_interval),
      m_on_timeout(on_timeout),
      m_sender(sender) {
}


/**
 * Send a E1.33 heartbeat
 */
void E133HealthCheckedConnection::SendHeartbeat() {
  OLA_INFO << "Sending heartbeat";

  // TODO(simon): fix this
  // This is a bit tricky because we need decent sequence number support here,
  // which means we need to be syncronized with the actual messages sent over
  // the tcp connection.

  ola::plugin::e131::OutgoingStreamTransport transport(m_descriptor);

  // no data in this PDU
  const ola::plugin::e131::RDMPDU pdu(NULL);
  ola::plugin::e131::E133Header header(
      "foo bar",
      0,
      0,
      false);  // rx_ack

  bool result = m_sender->SendRDM(header, &pdu, &transport);
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
