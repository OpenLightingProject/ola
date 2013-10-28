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
 * SimpleRpcController.h
 * Interface for a basic RPC Controller
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef COMMON_RPC_SIMPLERPCCONTROLLER_H_
#define COMMON_RPC_SIMPLERPCCONTROLLER_H_

#include <string>
#include "common/rpc/RpcController.h"

namespace ola {
namespace rpc {

class SimpleRpcController: public RpcController {
  public:
    SimpleRpcController();
    ~SimpleRpcController() {}

    void Reset();
    bool Failed() const { return m_failed; }
    std::string ErrorText() const { return m_error_text; }
    void StartCancel();

    void SetFailed(const std::string &reason);
    bool IsCanceled() const { return m_cancelled; }
    void NotifyOnCancel(CancelCallback *callback) {
      m_callback = callback;
    }

  private:
    bool m_failed;
    bool m_cancelled;
    std::string m_error_text;
    CancelCallback *m_callback;
};
}  // namespace rpc
}  // namespace ola

#endif  // COMMON_RPC_SIMPLERPCCONTROLLER_H_
