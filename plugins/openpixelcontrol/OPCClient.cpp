/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * OPCClient.cpp
 * The Open Pixel Control Client.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/openpixelcontrol/OPCClient.h"

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/io/BigEndianStream.h"
#include "ola/io/IOQueue.h"
#include "ola/io/NonBlockingSender.h"
#include "ola/network/SocketAddress.h"
#include "ola/util/Utils.h"
#include "plugins/openpixelcontrol/OPCConstants.h"

namespace ola {
namespace plugin {
namespace openpixelcontrol {

using ola::TimeInterval;
using ola::network::TCPSocket;

OPCClient::OPCClient(ola::io::SelectServerInterface *ss,
                     const ola::network::IPV4SocketAddress &target)
    : m_ss(ss),
      m_target(target),
      m_backoff(TimeInterval(1, 0), TimeInterval(300, 0)),
      m_pool(OPC_FRAME_SIZE),
      m_socket_factory(NewCallback(this, &OPCClient::SocketConnected)),
      m_tcp_connector(ss, &m_socket_factory, TimeInterval(3, 0)) {
  m_tcp_connector.AddEndpoint(target, &m_backoff);
}

OPCClient::~OPCClient() {
  if (m_client_socket.get()) {
    m_ss->RemoveReadDescriptor(m_client_socket.get());
    m_tcp_connector.Disconnect(m_target, true);
  }
}

bool OPCClient::SendDmx(uint8_t channel, const DmxBuffer &buffer) {
  if (!m_sender.get()) {
    return false;  // not connected
  }

  ola::io::IOQueue queue(&m_pool);
  ola::io::BigEndianOutputStream stream(&queue);
  stream << channel;
  stream << SET_PIXEL_COMMAND;
  stream << static_cast<uint16_t>(buffer.Size());
  stream.Write(buffer.GetRaw(), buffer.Size());
  return m_sender->SendMessage(&queue);
}

void OPCClient::SetSocketCallback(SocketEventCallback *callback) {
  m_socket_callback.reset(callback);
}

void OPCClient::SocketConnected(TCPSocket *socket) {
  m_client_socket.reset(socket);
  m_client_socket->SetOnData(NewCallback(this, &OPCClient::NewData));
  m_client_socket->SetOnClose(
      NewSingleCallback(this, &OPCClient::SocketClosed));
  m_ss->AddReadDescriptor(socket);

  m_sender.reset(
      new ola::io::NonBlockingSender(socket, m_ss, &m_pool, OPC_FRAME_SIZE));
  if (m_socket_callback.get()) {
    m_socket_callback->Run(true);
  }
}

void OPCClient::NewData() {
  // The OPC protocol seems to be unidirectional. The other clients don't even
  // bother reading from the socket.
  // Rather than letting the data buffer we read and discard any incoming data
  // here.
  OLA_WARN << "Received unexpected data from " << m_target;
  uint8_t discard[512];
  unsigned int data_received;
  m_client_socket->Receive(discard, arraysize(discard), data_received);
}

void OPCClient::SocketClosed() {
  m_sender.reset();
  m_client_socket.reset();

  if (m_socket_callback.get()) {
    m_socket_callback->Run(false);
  }
}
}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
