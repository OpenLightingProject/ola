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
 * SimpleClient.cpp
 * Implementation of Simple Client
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <lla/BaseTypes.h>
#include <lla/Logging.h>
#include <lla/SimpleClient.h>

namespace lla {


SimpleClient::SimpleClient():
  m_client(NULL),
  m_ss(NULL),
  m_socket(NULL) {
}

SimpleClient::~SimpleClient() {
  Cleanup();
}


/*
 * Setup the Simple Client
 *
 * @returns true on success, false on failure
 */
bool SimpleClient::Setup() {
  if (!m_ss)
    m_ss = new SelectServer();

  if (!m_socket) {
    m_socket = TcpSocket::Connect("127.0.0.1", LLA_DEFAULT_PORT);
    if (!m_socket) {
      delete m_socket;
      delete m_ss;
      m_socket = NULL;
      m_ss = NULL;
      return false;
    }
    m_socket->SetOnClose(
        lla::NewSingleClosure(this, &SimpleClient::SocketClosed));
  }

  if (!m_client) {
    m_client = new LlaClient(m_socket);
  }
  m_ss->AddSocket(m_socket);
  return m_client->Setup();
}


/*
 * Close the lla connection.
 *
 * @return true on sucess, false on failure
 */
bool SimpleClient::Cleanup() {
  if (m_client)
    delete m_client;

  if (m_socket)
    delete m_socket;

  if (m_ss)
    delete m_ss;
  return true;
}

/*
 * Called if the server closed the connection
 */
int SimpleClient::SocketClosed() {
  LLA_INFO << "Server closed the connection";
  m_ss->Terminate();
  return 0;
}

} // lla
