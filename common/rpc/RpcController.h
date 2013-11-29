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
 * RpcController.h
 * Interface for a basic RPC Controller.
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef COMMON_RPC_RPCCONTROLLER_H_
#define COMMON_RPC_RPCCONTROLLER_H_

#include <ola/Callback.h>
#include <string>

namespace ola {
namespace rpc {

class RpcController {
 public:
    RpcController();
    ~RpcController() {}

    void Reset();
    bool Failed() const { return m_failed; }
    std::string ErrorText() const { return m_error_text; }

    void SetFailed(const std::string &reason);

 private:
    bool m_failed;
    std::string m_error_text;
};
}  // namespace rpc
}  // namespace ola

#endif  // COMMON_RPC_RPCCONTROLLER_H_
