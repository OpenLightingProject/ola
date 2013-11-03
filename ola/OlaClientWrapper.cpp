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
 * OlaClientWrapper.cpp
 * Implementation of the OLA Client
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <ola/OlaClientWrapper.h>

#include <ola/BaseTypes.h>
#include <ola/Logging.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>

namespace ola {
namespace api {

BaseClientWrapper::~BaseClientWrapper() {
  Cleanup();
}

/*
 * Setup the Simple Client
 * @returns true on success, false on failure
 */
bool BaseClientWrapper::Setup() {
  if (!m_socket.get()) {
    InitSocket();

    if (!m_socket.get())
      return false;
  }

  CreateClient();

  m_ss.AddReadDescriptor(m_socket.get());
  return StartupClient();
}


/*
 * Close the ola connection.
 * @return true on sucess, false on failure
 */
bool BaseClientWrapper::Cleanup() {
  if (m_socket.get())
    m_socket->Close();
  return true;
}

/*
 * Called if the server closed the connection
 */
void BaseClientWrapper::SocketClosed() {
  OLA_INFO << "Server closed the connection";
  m_ss.Terminate();
}
}  // namespace api
}  // namespace ola
