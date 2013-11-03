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
 */
/**
 * @file
 * @brief A client for sending DMX512 data to olad.
 */

#ifndef INCLUDE_OLA_CLIENT_STREAMINGCLIENT_H_
#define INCLUDE_OLA_CLIENT_STREAMINGCLIENT_H_

#include <ola/BaseTypes.h>
#include <ola/DmxBuffer.h>
#include <ola/base/Macro.h>

namespace ola {

namespace io { class SelectServer; }
namespace network { class TCPSocket; }
namespace proto { class OlaServerService_Stub; }
namespace rpc { class RpcChannel; }

namespace client {

/**
 * @class StreamingClient ola/client/StreamingClient.h
 * @brief Send DMX512 data to olad.
 *
 * StreamingClient sends DMX512 data to OLAD without waiting for an
 * acknowledgement. It's best suited to simple clients which only ever send
 * DMX512 data.
 *
 * @snippet streaming_client.cpp Tutorial Example
 */
class StreamingClient {
  public:
    /**
     * Controls the options for the StreamingClient class.
     */
    class Options {
      public:
        /**
         * Create a new options structure with the default options. This
         * includes automatically starting olad if it's not already running.
         */
        Options() : auto_start(true), server_port(OLA_DEFAULT_PORT) {}

        /**
         * If true, the client will automatically start olad if it's not
         * already running.
         */
        bool auto_start;

        /**
         * The RPC port olad is listening on.
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

    void ChannelClosed();

  private:
    bool m_auto_start;
    uint16_t m_server_port;
    ola::network::TCPSocket *m_socket;
    ola::io::SelectServer *m_ss;
    class ola::rpc::RpcChannel *m_channel;
    class ola::proto::OlaServerService_Stub *m_stub;
    bool m_socket_closed;

    DISALLOW_COPY_AND_ASSIGN(StreamingClient);
};
}  // namespace client
}  // namespace ola
#endif  // INCLUDE_OLA_CLIENT_STREAMINGCLIENT_H_
