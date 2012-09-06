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
 * StreamingClient.h
 * Interface to the Streaming Client class.
 * Copyright (C) 2005-2010 Simon Newton
 *
 * This client does one thing and one thing only. Sends DMX data to a OLA
 * server. It doesn't support any callbacks. This is very useful in integrating
 * OLA into programs like QLC and Max.
 */

#ifndef OLA_STREAMINGCLIENT_H_
#define OLA_STREAMINGCLIENT_H_

#include <ola/DmxBuffer.h>
#include <ola/io/SelectServer.h>
#include <ola/network/TCPSocket.h>

namespace ola {

namespace rpc {
  class StreamRpcChannel;
}

namespace proto {
  class OlaServerService_Stub;
}

using ola::io::SelectServer;
using ola::network::TCPSocket;

/*
 * StreamingClient opens a connection and then sends data over the socket.
 */
class StreamingClient {
  public:
    explicit StreamingClient(bool auto_start = true);
    ~StreamingClient();

    bool Setup();
    void Stop();

    bool SendDmx(unsigned int universe, const DmxBuffer &data);
    void SocketClosed();

  private:
    StreamingClient(const StreamingClient&);
    StreamingClient operator=(const StreamingClient&);

    bool m_auto_start;
    TCPSocket *m_socket;
    SelectServer *m_ss;
    class ola::rpc::StreamRpcChannel *m_channel;
    class ola::proto::OlaServerService_Stub *m_stub;
    bool m_socket_closed;
};
}  // ola
#endif  // OLA_STREAMINGCLIENT_H_
