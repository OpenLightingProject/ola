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
 * E133TCPConnector.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_E133_E133TCPCONNECTOR_H_
#define TOOLS_E133_E133TCPCONNECTOR_H_

#include <ola/network/AdvancedTCPConnector.h>

using ola::network::AdvancedTCPConnector;


/**
 * The E1.33 TCP connector is slighty different, because a ECONNREFUSED means
 * the device is locked by another Controller. Rather than retrying, we should
 * wait for a signal that the lock has been released.
 */
class E133TCPConnector: public ola::network::AdvancedTCPConnector {
  public:
    E133TCPConnector(ola::network::SelectServerInterface *ss,
                     OnConnect *on_connect,
                     const ola::TimeInterval &connection_timeout)
        : AdvancedTCPConnector(ss, on_connect, connection_timeout) {
    }

  protected:
    void TakeAction(const AdvancedTCPConnector::IPPortPair &key,
                    AdvancedTCPConnector::ConnectionInfo *info,
                    ola::network::TcpSocket *socket,
                    int error);
};
#endif  // TOOLS_E133_E133TCPCONNECTOR_H_
