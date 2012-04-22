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
 * E133HealthCheckedConnection.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_
#define TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/network/HealthCheckedConnection.h>
#include "tools/e133/E133StreamSender.h"


/**
 * An E1.33 health checked connection.
 */
class E133HealthCheckedConnection
    : public ola::network::HealthCheckedConnection {
  public:
    E133HealthCheckedConnection(
        E133StreamSender *e133_sender,
        ola::SingleUseCallback0<void> *on_timeout,
        ola::thread::SchedulerInterface *scheduler,
        const ola::TimeInterval timeout_interval =
          ola::TimeInterval(E133_HEARTBEAT_INTERVAL, 0));

    ~E133HealthCheckedConnection() {}

    void SendHeartbeat();
    void HeartbeatTimeout();

  private:
    ola::SingleUseCallback0<void> *m_on_timeout;
    E133StreamSender *m_sender;

    // The default interval in seconds for sending heartbeat messages.
    static const unsigned int E133_HEARTBEAT_INTERVAL = 2;
};
#endif  // TOOLS_E133_E133HEALTHCHECKEDCONNECTION_H_
