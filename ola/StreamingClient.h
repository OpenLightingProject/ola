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

#include <ola/BaseTypes.h>
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

/**
 * @class StreamingClient ola/StreamingClient.h
 * @brief Send DMX512 data to olad.
 *
 * StreamingClient sends DMX512 data to OLAD without waiting for an
 * acknowledgement. It's best used for simple clients which only ever send
 * DMX512 data.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~
 *  unsigned int universe = 1;
 *  ola::DmxBuffer dmx_data;
 *  dmx_data.Blackout();
 *  ola::StreamingClient client(ola::StreamingClient::Options());
 *
 *  if (!client.SendDmx(universe, dmx_data)) {
 *    // Failed to send
 *    ...
 *  }
 * ~~~~~~~~~~~~~~~~~~~~~
 */
class StreamingClient {
  public:
    /**
     *
     */
    struct Options {
      public:
        /**
         * Create a new options structure with the default options.
         */
        Options() : auto_start(true), server_port(OLA_DEFAULT_PORT) {}

        /**
         * Automatically start olad if it's not already running.
         */
        bool auto_start;
        /**
         * Specifiy the olad RPC port to connect to.
         */
        uint16_t server_port;
    };

    /**
     * Create a new StreamingClient.
     * @param auto_start if set to true, this will automatically start olad if
     *   it's not already running.
     * @deprecated Use the constructor that takes an Options struct instead.
     */
    explicit StreamingClient(bool auto_start = true);

    /**
     * Create a new StreamingClient.
     * @param options an Options structure.
     */
    explicit StreamingClient(const Options &options);

    /**
     * Destructor. This closes the connection to the olad server if it's still
     * open.
     */
    ~StreamingClient();

    /**
     * Initialize the client and connect to olad.
     * @returns true if the initialization completed sucessfully, false if
     *   there was a failure.
     */
    bool Setup();

    /**
     * Close the connection to the olad server. This does not need to be called
     * since ~StreamingClient() will close the connection if it's still open
     * when the object is destroyed.
     */
    void Stop();

    /**
     * Send a DmxBuffer to the olad server.
     * @returns true if sent sucessfully, false if the connection to the server
     *   has been closed.
     */
    bool SendDmx(unsigned int universe, const DmxBuffer &data);

    void SocketClosed();

  private:
    StreamingClient(const StreamingClient&);
    StreamingClient operator=(const StreamingClient&);

    bool m_auto_start;
    uint16_t m_server_port;
    TCPSocket *m_socket;
    SelectServer *m_ss;
    class ola::rpc::StreamRpcChannel *m_channel;
    class ola::proto::OlaServerService_Stub *m_stub;
    bool m_socket_closed;
};
}  // namespace ola
#endif  // OLA_STREAMINGCLIENT_H_
