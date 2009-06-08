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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SimpleClient.h
 * Interface to the Simple Client class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_SIMPLE_CLIENT_H
#define LLA_SIMPLE_CLIENT_H

#include <lla/LlaClient.h>
#include <lla/network/SelectServer.h>
#include <lla/network/Socket.h>

namespace lla {

using lla::network::SelectServer;
using lla::network::TcpSocket;

/*
 * SimpleClient takes care of setting up the socket, select server and client
 * for you.
 */
class SimpleClient {
  public:
    SimpleClient();
    ~SimpleClient();

    LlaClient *GetClient() const { return m_client; }
    SelectServer *GetSelectServer() const { return m_ss; }

    bool Setup();
    bool Cleanup();

    int SocketClosed();

  private:
    SimpleClient(const SimpleClient&);
    SimpleClient operator=(const SimpleClient&);

    LlaClient *m_client;
    SelectServer *m_ss;
    TcpSocket *m_socket;
};

} // lla

#endif
