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
 * Implementation of Simple Client
 * Copyright (C) 2005-2008 Simon Newton
 */


#include <ola/AutoStart.h>
#include <ola/BaseTypes.h>
#include <ola/Logging.h>
#include <ola/OlaClientWrapper.h>

namespace ola {

BaseClientWrapper::BaseClientWrapper(bool auto_start)
    : m_socket(NULL),
      m_auto_start(auto_start),
      m_ss(NULL) {
}


BaseClientWrapper::~BaseClientWrapper() {
  Cleanup();
}


/*
 * Setup the Simple Client
 * @returns true on success, false on failure
 */
bool BaseClientWrapper::Setup() {
  if (!m_ss)
    m_ss = new SelectServer();

  if (!m_socket) {
    if (m_auto_start)
      m_socket = ola::client::ConnectToServer(OLA_DEFAULT_PORT);
    else
    m_socket = TcpSocket::Connect("127.0.0.1", OLA_DEFAULT_PORT);

    if (!m_socket) {
      delete m_socket;
      delete m_ss;
      m_socket = NULL;
      m_ss = NULL;
      return false;
    }
    m_socket->SetOnClose(
        ola::NewSingleCallback(this, &OlaClientWrapper::SocketClosed));
  }

  CreateClient();
  m_ss->AddReadDescriptor(m_socket);
  return StartupClient();
}


/*
 * Close the ola connection.
 * @return true on sucess, false on failure
 */
bool BaseClientWrapper::Cleanup() {
  if (m_socket)
    delete m_socket;

  if (m_ss)
    delete m_ss;
  return true;
}

/*
 * Called if the server closed the connection
 */
void BaseClientWrapper::SocketClosed() {
  OLA_INFO << "Server closed the connection";
  m_ss->Terminate();
}
}  // ola
