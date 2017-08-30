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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * OpcServer.h
 * The Open Pixel Server.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_OPENPIXELCONTROL_OPCSERVER_H_
#define PLUGINS_OPENPIXELCONTROL_OPCSERVER_H_

#include <string>
#include <map>
#include <memory>
#include "ola/Constants.h"
#include "ola/DmxBuffer.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/network/SocketAddress.h"
#include "ola/network/TCPSocket.h"
#include "ola/network/TCPSocketFactory.h"
#include "plugins/openpixelcontrol/OpcConstants.h"

namespace ola {
namespace plugin {
namespace openpixelcontrol {

/**
 * @brief An Open Pixel Control server.
 *
 * The server listens on a TCP port and receives OPC data.
 */
class OpcServer {
 public:
  /**
   * @brief The callback executed when new OPC data arrives.
   */
  typedef Callback3<void, uint8_t, const uint8_t*, unsigned int>
      ChannelCallback;

  /**
   * @brief Create a new OpcServer.
   * @param ss The SelectServer to use
   * @param listen_addr the IP:port to listen on.
   */
  OpcServer(ola::io::SelectServerInterface *ss,
            const ola::network::IPV4SocketAddress &listen_addr);

  /**
   * @brief Destructor.
   */
  ~OpcServer();

  /**
   * @brief Initialize the OpcServer.
   * @returns true if the server is now listening for new connections, false
   *   otherwise.
   */
  bool Init();

  /**
   * @brief Set the callback to be run when channel data arrives.
   * @param channel the OPC channel this callback is for.
   * @param callback The callback to run, ownership is transferred and any
   *   previous callbacks for this channel are removed.
   */
  void SetCallback(uint8_t channel, ChannelCallback *callback);

  /**
   * @brief The listen address of this server
   * @returns The listen address of the server. If the server isn't listening
   *   the empty string is returned.
   */
  ola::network::IPV4SocketAddress ListenAddress() const;

 private:
  struct RxState {
   public:
    unsigned int offset;
    uint16_t expected_size;
    uint8_t *data;
    unsigned int buffer_size;

    RxState()
        : offset(0),
          expected_size(0) {
      buffer_size = OPC_FRAME_SIZE,
      data = new uint8_t[buffer_size];
    }

    ~RxState() {
      delete[] data;
    }

    void CheckSize();
  };

  typedef std::map<ola::network::TCPSocket*, RxState*> ClientMap;

  ola::io::SelectServerInterface* const m_ss;
  const ola::network::IPV4SocketAddress m_listen_addr;
  ola::network::TCPSocketFactory m_tcp_socket_factory;

  std::auto_ptr<ola::network::TCPAcceptingSocket> m_listening_socket;
  ClientMap m_clients;
  std::map<uint8_t, ChannelCallback*> m_callbacks;

  void NewTCPConnection(ola::network::TCPSocket *socket);
  void SocketReady(ola::network::TCPSocket *socket, RxState *rx_state);
  void SocketClosed(ola::network::TCPSocket *socket);

  DISALLOW_COPY_AND_ASSIGN(OpcServer);
};
}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OPENPIXELCONTROL_OPCSERVER_H_
