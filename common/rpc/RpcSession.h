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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * RpcSession.h
 * The RPC Session object.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef COMMON_RPC_RPCSESSION_H_
#define COMMON_RPC_RPCSESSION_H_

namespace ola {
namespace rpc {

class RpcChannel;

/**
 * @brief Represents the RPC session between a client and server.
 *
 * The RPCSession object contains information about the session an RPC is part
 * of. For each RPC method on the server side, the RPCSession can be accessed
 * via the RpcController::Session() method.
 *
 * Sessions can have arbitrary user data associated with them, similar to a
 * cookie in an HTTP request. The user data is usually set in the call to
 * RpcSessionHandlerInterface::NewClient() but can be set or modified in any of
 * the RPC calls themselves.
 *
 * Since the RPCSession object doesn't take ownership of the user data, its
 * should be deleted in the call to
 * RpcSessionHandlerInterface::ClientRemoved().
 */
class RpcSession {
 public:
  /**
   * @brief Create a new session object.
   * @param channel the RpcChannel that the session is using. Ownership is not
   *   transferred.
   */
  explicit RpcSession(RpcChannel *channel)
      : m_channel(channel),
        m_data(NULL) {
  }

  /**
   * @brief Returns the underlying RPCChannel
   * @returns The RPCChannel that corresponds to this session.
   */
  RpcChannel *Channel() { return m_channel; }

  /**
   * @brief Associate user data with this session.
   * @param ptr Opaque user data to associate with this session. Ownership is
   *   not transferred.
   */
  void SetData(void *ptr) {
    m_data = ptr;
  }

  /**
   * @brief Retrieve the user data associated with this session.
   * @returns The user data associated with the session.
   */
  void *GetData() const { return m_data; }

  // TODO(simon): return the RpcPeer here as well.

 private:
  RpcChannel *m_channel;
  void *m_data;
};
}  // namespace rpc
}  // namespace ola
#endif  // COMMON_RPC_RPCSESSION_H_
