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
 * The RpcController.
 * Copyright (C) 2005 Simon Newton
 */

#include "common/rpc/RpcController.h"

#include <string>
#include "common/rpc/RpcSession.h"

namespace ola {
namespace rpc {

RpcController::RpcController(RpcSession *session)
    : m_session(session),
      m_failed(false),
      m_error_text("") {
}

void RpcController::Reset() {
  m_failed = false;
  m_error_text = "";
}

void RpcController::SetFailed(const std::string &reason) {
  m_failed = true;
  m_error_text = reason;
}

RpcSession *RpcController::Session() {
  return m_session;
}
}  // namespace rpc
}  // namespace ola
