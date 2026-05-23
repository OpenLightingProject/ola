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
 * OPCClient.h
 * The Open Pixel Control Client.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_OPENPIXELCONTROL_OPCCLIENT_H_
#define PLUGINS_OPENPIXELCONTROL_OPCCLIENT_H_

#include <memory>
#include <string>

#include "ola/DmxBuffer.h"
#include "ola/io/MemoryBlockPool.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/network/AdvancedTCPConnector.h"
#include "ola/network/SocketAddress.h"
#include "ola/network/TCPSocket.h"
#include "ola/util/Backoff.h"

namespace ola {

namespace io {
class NonBlockingSender;
}  // namespace io

namespace plugin {
namespace openpixelcontrol {

/**
 * @brief An Open Pixel Control client.
 *
 * The OPC client connects to a remote IP:port and sends OPC messages.
 */
class OPCClient {
 public:
  /**
   * @brief Called when the socket changes state.
   */
  typedef ola::Callback1<void, bool> SocketEventCallback;

  /**
   * @brief Create a new OPCClient.
   * @param ss The SelectServer to use
   * @param target the remote IP:port to connect to.
   */
  OPCClient(ola::io::SelectServerInterface *ss,
            const ola::network::IPV4SocketAddress &target);

  /**
   * @brief Destructor.
   */
  ~OPCClient();

  /**
   * @brief Return the remote address for this Client.
   * @returns An IP:port as a string.
   */
  std::string GetRemoteAddress() const { return m_target.ToString(); }

  /**
   * @brief Send a DMX frame.
   * @param channel the OPC channel to use.
   * @param buffer the DMX data.
   */
  bool SendDmx(uint8_t channel, const DmxBuffer &buffer);

  /**
   * @brief Set the callback to be run when the socket state changes.
   * @param callback the callback to run when the socket state changes.
   *   Ownership is transferred.
   */
  void SetSocketCallback(SocketEventCallback *callback);

 private:
  ola::io::SelectServerInterface *m_ss;
  const ola::network::IPV4SocketAddress m_target;

  ola::ExponentialBackoffPolicy m_backoff;
  ola::io::MemoryBlockPool m_pool;
  ola::network::TCPSocketFactory m_socket_factory;
  ola::network::AdvancedTCPConnector m_tcp_connector;
  std::unique_ptr<ola::network::TCPSocket> m_client_socket;
  std::unique_ptr<ola::io::NonBlockingSender> m_sender;
  std::unique_ptr<SocketEventCallback> m_socket_callback;

  void SocketConnected(ola::network::TCPSocket *socket);
  void NewData();
  void SocketClosed();

  DISALLOW_COPY_AND_ASSIGN(OPCClient);
};
}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OPENPIXELCONTROL_OPCCLIENT_H_
