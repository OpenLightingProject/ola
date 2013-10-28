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
    typedef Callback0<void> CancelCallback;

    RpcController() {}
    virtual ~RpcController() {}

    virtual void Reset() = 0;
    virtual bool Failed() const = 0;
    virtual std::string ErrorText() const = 0;
    virtual void StartCancel() = 0;

    virtual void SetFailed(const std::string &reason) = 0;
    virtual bool IsCanceled() const = 0;
    virtual void NotifyOnCancel(CancelCallback *callback)  = 0;
};
}  // namespace rpc
}  // namespace ola

#endif  // COMMON_RPC_RPCCONTROLLER_H_
