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
 * OpcServer.cpp
 * The Open Pixel Server.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/openpixelcontrol/OpcServer.h"

#include <string>
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/network/SocketAddress.h"
#include "ola/stl/STLUtils.h"
#include "ola/util/Utils.h"

namespace ola {
namespace plugin {
namespace openpixelcontrol {

using ola::network::GenericSocketAddress;
using ola::network::IPV4SocketAddress;
using ola::network::TCPAcceptingSocket;
using ola::network::TCPSocket;
using std::string;

namespace {

void CleanupSocket(TCPSocket *socket) {
  delete socket;
}
}  // namespace

void OpcServer::RxState::CheckSize() {
  expected_size = utils::JoinUInt8(data[2], data[3]);
  if (static_cast<unsigned int>(expected_size) + OPC_HEADER_SIZE >
      buffer_size) {
    uint8_t *new_buffer = new uint8_t[expected_size + OPC_HEADER_SIZE];
    memcpy(new_buffer, data, offset);
    delete[] data;
    data = new_buffer;
    buffer_size = expected_size + OPC_HEADER_SIZE;
  }
}

OpcServer::OpcServer(ola::io::SelectServerInterface *ss,
                     const ola::network::IPV4SocketAddress &listen_addr)
    : m_ss(ss),
      m_listen_addr(listen_addr),
      m_tcp_socket_factory(
          ola::NewCallback(this, &OpcServer::NewTCPConnection)) {
}

OpcServer::~OpcServer() {
  if (m_listening_socket.get()) {
    m_ss->RemoveReadDescriptor(m_listening_socket.get());
    m_listening_socket.reset();
  }

  ClientMap::iterator iter = m_clients.begin();
  for (; iter != m_clients.end(); ++iter) {
    m_ss->RemoveReadDescriptor(iter->first);
    delete iter->first;
    delete iter->second;
  }

  STLDeleteValues(&m_callbacks);
}

bool OpcServer::Init() {
  std::auto_ptr<TCPAcceptingSocket> listening_socket(
      new TCPAcceptingSocket(&m_tcp_socket_factory));
  if (!listening_socket->Listen(m_listen_addr)) {
    return false;
  }
  m_ss->AddReadDescriptor(listening_socket.get());
  m_listening_socket.reset(listening_socket.release());
  return true;
}

IPV4SocketAddress OpcServer::ListenAddress() const {
  if (m_listening_socket.get()) {
    GenericSocketAddress addr = m_listening_socket->GetLocalAddress();
    if (addr.Family() == AF_INET) {
      return addr.V4Addr();
    }
  }
  return IPV4SocketAddress();
}

void OpcServer::SetCallback(uint8_t channel, ChannelCallback *callback) {
  STLReplaceAndDelete(&m_callbacks, channel, callback);
}

void OpcServer::NewTCPConnection(TCPSocket *socket) {
  if (!socket)
    return;

  RxState *rx_state = new RxState();

  socket->SetNoDelay();
  socket->SetOnData(
      NewCallback(this, &OpcServer::SocketReady, socket, rx_state));
  socket->SetOnClose(
      NewSingleCallback(this, &OpcServer::SocketClosed, socket));
  m_ss->AddReadDescriptor(socket);
  STLReplaceAndDelete(&m_clients, socket, rx_state);
}

void OpcServer::SocketReady(TCPSocket *socket, RxState *rx_state) {
  unsigned int data_received = 0;
  if (socket->Receive(rx_state->data + rx_state->offset,
                      rx_state->buffer_size - rx_state->offset,
                      data_received) < 0) {
    OLA_WARN << "Bad read from " << socket->GetPeerAddress();
    SocketClosed(socket);
    return;
  }

  rx_state->offset += data_received;
  if (rx_state->offset < OPC_HEADER_SIZE) {
    return;
  }

  rx_state->CheckSize();
  if (rx_state->offset <
      static_cast<unsigned int>(rx_state->expected_size) + OPC_HEADER_SIZE) {
    return;
  }

  ChannelCallback *cb = STLFindOrNull(m_callbacks, rx_state->data[0]);
  if (cb) {
    DmxBuffer buffer(rx_state->data + OPC_HEADER_SIZE,
                     rx_state->offset - OPC_HEADER_SIZE);
    cb->Run(rx_state->data[1],
            rx_state->data + OPC_HEADER_SIZE,
            rx_state->expected_size);
  }
  rx_state->offset = 0;
  rx_state->expected_size = 0;
}

void OpcServer::SocketClosed(TCPSocket *socket) {
  m_ss->RemoveReadDescriptor(socket);
  STLRemoveAndDelete(&m_clients, socket);

  // Since we're in the call stack of the socket, we schedule deletion during
  // the next run of the event loop to break out of the stack.
  m_ss->Execute(NewSingleCallback(&CleanupSocket, socket));
}
}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
