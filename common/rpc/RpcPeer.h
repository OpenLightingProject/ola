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
 * RpcPeer.h
 * Represents the peer of an RpcChannel.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef COMMON_RPC_RPCPEER_H_
#define COMMON_RPC_RPCPEER_H_

#include <ola/Callback.h>
#include <ola/network/SocketAddress.h>
#include <string>

namespace ola {
namespace rpc {

class RpcPeer {
 public:
  explicit RpcPeer(const ola::network::GenericSocketAddress &socket_addr)
    : m_socket_addr(socket_addr) {
  }

  const ola::network::GenericSocketAddress& SocketAddress() const {
    return m_socket_addr;
  }

  /**
   * @brief Assignment operator
   */
  RpcPeer& operator=(const RpcPeer& other) {
    if (this != &other) {
      m_socket_addr = other.m_socket_addr;
    }
    return *this;
  }

  /**
   * @brief Convert a peer to a human readable string.
   * @returns a string representation of the peer.
   */
  std::string ToString() const {
    return m_socket_addr.ToString();
  }

  /**
   * @brief A helper function to write an RpcPeer to an ostream.
   * @param out the ostream
   * @param peer the RpcPeer to write.
   */
  friend ostream& operator<<(ostream &out, const RpcPeer &peer) {
    return out << peer.ToString();
  }

 private:
  ola::network::GenericSocketAddress m_socket_addr;
};
}  // namespace rpc
}  // namespace ola

#endif  // COMMON_RPC_RPCPEER_H_
