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
 * OlaClientWrapper.cpp
 * Implementation of the OLA Client
 * Copyright (C) 2005 Simon Newton
 */

#include <ola/OlaClientWrapper.h>

#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>

namespace ola {
namespace client {

BaseClientWrapper::BaseClientWrapper()
  : m_close_callback(NewCallback(&m_ss, &ola::io::SelectServer::Terminate)) {
}

BaseClientWrapper::~BaseClientWrapper() {
  Cleanup();
}

void BaseClientWrapper::SetCloseCallback(CloseCallback *callback) {
  m_close_callback.reset(callback);
}

bool BaseClientWrapper::Setup() {
  if (!m_socket.get()) {
    InitSocket();

    if (!m_socket.get()) {
      return false;
    }
  }

  CreateClient();

  if (!m_ss.AddReadDescriptor(m_socket.get())) {
    return false;
  }

  return StartupClient();
}


bool BaseClientWrapper::Cleanup() {
  if (m_socket.get()) {
    m_socket->Close();
    m_socket.reset();
  }
  return true;
}

void BaseClientWrapper::SocketClosed() {
  OLA_INFO << "Server closed the connection";
  m_close_callback->Run();
}
}  // namespace client
}  // namespace ola
