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
 * RpcController.h
 * The RpcController.
 * Copyright (C) 2005 Simon Newton
 */

#ifndef COMMON_RPC_RPCCONTROLLER_H_
#define COMMON_RPC_RPCCONTROLLER_H_

#include <ola/Callback.h>
#include <string>

namespace ola {
namespace rpc {

class RpcSession;

/**
 * @brief A RpcController object is passed every time an RPC is invoked and is
 * used to indicate the success or failure of the RPC.
 *
 * On the client side, the controller can is used once the callback completes to
 * check the outcome of the RPC with Failed(). If the RPC failed, a description
 * of the error is available by calling ErrorText().
 *
 * On the server side, the server can fail the RPC by calling SetFailed(...).
 */
class RpcController {
 public:
  /**
   * @brief Create a new RpcController
   * @param session the RpcSession to use. Ownership is not transferred.
   */
  explicit RpcController(RpcSession *session = NULL);
  ~RpcController() {}

  /**
   * @brief Reset the state of this controller. Does not affect the session.
   */
  void Reset();

  /**
   * @brief Check if the RPC call this controller was associated with failed.
   * @returns true if the RPC failed, false if the RPC succeeded.
   */
  bool Failed() const { return m_failed; }

  /**
   * @brief Return the error string if the RPC failed.
   * @returns the error text, or the empty string if the RPC succeeded.
   */
  std::string ErrorText() const { return m_error_text; }

  /**
   * @brief Mark this RPC as failed.
   * @param reason the string to return in ErrorText().
   */
  void SetFailed(const std::string &reason);

  /**
   * @brief Get the session information for this RPC.
   *
   * Unless specifically provided, the session be NULL on the client side.
   * @returns the RpcSession object, ownership is not transferred.
   */
  RpcSession *Session();

 private:
  RpcSession *m_session;
  bool m_failed;
  std::string m_error_text;
};
}  // namespace rpc
}  // namespace ola

#endif  // COMMON_RPC_RPCCONTROLLER_H_
