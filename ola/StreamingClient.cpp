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
 * StreamingClient.cpp
 * Implementation of Streaming Client
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <ola/BaseTypes.h>
#include <ola/Closure.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/StreamingClient.h>
#include "common/protocol/Ola.pb.h"
#include "common/rpc/StreamRpcChannel.h"

namespace ola {

using ola::rpc::StreamRpcChannel;
using ola::proto::OlaServerService_Stub;

StreamingClient::StreamingClient()
    : m_socket(NULL),
      m_closure(NULL),
      m_channel(NULL),
      m_stub(NULL) {
}

StreamingClient::~StreamingClient() {
  Stop();
}


/*
 * Setup the Streaming Client
 *
 * @returns true on success, false on failure
 */
bool StreamingClient::Setup() {
  if (!m_socket) {
    m_socket = TcpSocket::Connect("127.0.0.1", OLA_DEFAULT_PORT);
    if (!m_socket) {
      return false;
    }
  }

  m_socket->SetOnClose(
      NewSingleClosure(this, &StreamingClient::SocketClosed));

  m_channel = new StreamRpcChannel(NULL, m_socket);

  if (!m_channel) {
    delete m_socket;
    m_socket = NULL;
    return false;
  }

  m_stub = new OlaServerService_Stub(m_channel);

  if (!m_stub) {
    delete m_channel;
    delete m_socket;
    m_channel = NULL;
    m_socket = NULL;
    return false;
  }
  return true;
}


/*
 * Close the ola connection.
 * @return true on sucess, false on failure
 */
void StreamingClient::Stop() {
  if (m_stub)
    delete m_stub;

  if (m_channel)
    delete m_channel;

  if (m_socket)
    delete m_socket;

  m_stub = NULL;
  m_channel = NULL;
  m_socket = NULL;
}


/*
 * Send DMX to the remote OLA server
 */
bool StreamingClient::SendDmx(unsigned int universe,
                              const DmxBuffer &data) const {
  if (!m_stub || m_socket->CheckIfClosed())
    return false;

  ola::proto::DmxData request;
  request.set_universe(universe);
  request.set_data(data.Get());
  m_stub->StreamDmxData(NULL, &request, NULL, NULL);
  return true;
}


/*
 * Called when the socket is closed
 */
int StreamingClient::SocketClosed() {
  OLA_WARN << "The RPC socket has been closed, this is more than likely due"
    << "to a framing error, perhaps you're sending too fast?";
  if (m_closure)
    m_closure->Run();
  return 0;
}
}  // ola
