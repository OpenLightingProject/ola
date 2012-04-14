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
 * E133TCPConnector.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <sys/errno.h>

#include "tools/e133/E133TCPConnector.h"


/**
 * Decide what to do when a connection fails, completes or times out.
 */
void E133TCPConnector::TakeAction(const AdvancedTCPConnector::IPPortPair &key,
                                  AdvancedTCPConnector::ConnectionInfo *info,
                                  ola::network::TcpSocket *socket,
                                  int error) {
  if (socket) {
    // connected, ok
    info->state = CONNECTED;
    m_on_connect->Run(key.first, key.second, socket);
  } else if (error == ECONNREFUSED) {
    // the device is locked
    info->state = PAUSED;
  } else {
    // error
    info->failed_attempts++;
    ScheduleRetry(key, info);
  }
}
