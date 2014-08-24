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
 * RpcController.cpp
 * The  RPC Controller
 * Copyright (C) 2005 Simon Newton
 */

#include <string>
#include "ola/Logging.h"
#include "common/rpc/RpcController.h"

namespace ola {
namespace rpc {

using std::string;

RpcController::RpcController()
    : m_failed(false),
      m_error_text("") {
}

void RpcController::Reset() {
  m_failed = false;
}

void RpcController::SetFailed(const string &reason) {
  m_failed = true;
  m_error_text = reason;
}
}  // namespace rpc
}  // namespace ola
