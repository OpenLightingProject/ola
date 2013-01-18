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
 * since it just sends Root PDUs as heartbeat messages.
 */

#ifndef TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_
#define TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/network/HealthCheckedConnection.h>
#include "plugins/e131/e131/E133Inflator.h"
#include "plugins/e131/e131/RootSender.h"
#include "plugins/e131/e131/TCPTransport.h"


/**
 * An E1.33 health checked connection.
 */
class E133HealthCheckedConnection
    : public ola::network::HealthCheckedConnection {
  public:
    E133HealthCheckedConnection(
        ola::plugin::e131::OutgoingStreamTransport *transport,
        ola::plugin::e131::RootSender *root_sender,
        ola::SingleUseCallback0<void> *on_timeout,
        ola::thread::SchedulerInterface *scheduler,
        unsigned int vector = ola::plugin::e131::E133Inflator::E133_VECTOR,
        const ola::TimeInterval heartbeat_interval =
          ola::TimeInterval(E133_HEARTBEAT_INTERVAL, 0));

    ~E133HealthCheckedConnection() {
      if (m_on_timeout && !m_in_timeout)
        delete m_on_timeout;
    }

    void SendHeartbeat();
    void HeartbeatTimeout();

  private:
    bool m_in_timeout;
    unsigned int m_vector;
    ola::plugin::e131::OutgoingStreamTransport *m_transport;
    ola::plugin::e131::RootSender *m_sender;
    ola::SingleUseCallback0<void> *m_on_timeout;

    // The default interval in seconds for sending heartbeat messages.
    static const unsigned int E133_HEARTBEAT_INTERVAL = 2;
};
#endif  // TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_
