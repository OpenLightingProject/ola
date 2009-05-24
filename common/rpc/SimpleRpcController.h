/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SimpleRpcController.h
 * Interface for a basic RPC Controller
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef LLA_SIMPLE_RPC_CONTROLLER_H
#define LLA_SIMPLE_RPC_CONTROLLER_H

#include <string>
#include <google/protobuf/service.h>

namespace lla {
namespace rpc {

class SimpleRpcController: public google::protobuf::RpcController {
  public:
    SimpleRpcController();
    ~SimpleRpcController() {}

    void Reset();
    bool Failed() const { return m_failed; }
    std::string ErrorText() const { return m_error_text; }
    void StartCancel();

    void SetFailed(const std::string &reason);
    bool IsCanceled() const { return m_cancelled; }
    void NotifyOnCancel(google::protobuf::Closure *callback) {
      m_callback = callback;
    }

  private:
    bool m_failed;
    bool m_cancelled;
    std::string m_error_text;
    google::protobuf::Closure *m_callback;
};

} // rpc
} // lla

#endif
