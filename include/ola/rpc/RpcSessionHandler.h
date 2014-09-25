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
 * RpcSessionHandler.h
 * The interface for the RpcSessionHandler objects.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef INCLUDE_OLA_RPC_RPCSESSIONHANDLER_H_
#define INCLUDE_OLA_RPC_RPCSESSIONHANDLER_H_

namespace ola {
namespace rpc {

class RpcSession;

/**
 * @brief Used to receive notifications of RPC client session activity.
 *
 * An object that implements the RpcSessionHandlerInterface can be passed to
 * the RpcServer in the RpcServer's constructor. When clients connect to the
 * RpcServer, NewClient() will be called with an RpcSession object.
 */
class RpcSessionHandlerInterface {
 public:
  virtual ~RpcSessionHandlerInterface() {}

  /**
   * @brief Called by the RpcServer when a client connects.
   * @param session the RpcSession for the client.
   */
  virtual void NewClient(RpcSession *session) = 0;

  /**
   * @brief Called by the RpcServer when a client disconnects.
   * @param session the RpcSession for the client.
   */
  virtual void ClientRemoved(RpcSession *session) = 0;
};

}  // namespace rpc
}  // namespace ola
#endif  // INCLUDE_OLA_RPC_RPCSESSIONHANDLER_H_
