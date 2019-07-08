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
 * RpcServer.cpp
 * A generic RPC server.
 * Copyright (C) 2014 Simon Newton
 */

#include "common/rpc/RpcServer.h"

#include <ola/ExportMap.h>
#include <ola/Logging.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/TCPSocket.h>
#include <ola/rpc/RpcSessionHandler.h>
#include "common/rpc/RpcChannel.h"
#include "common/rpc/RpcSession.h"

namespace ola {
namespace rpc {

using std::auto_ptr;
using ola::io::ConnectedDescriptor;
using ola::network::GenericSocketAddress;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPAcceptingSocket;
using ola::network::TCPSocket;

namespace {
void CleanupChannel(RpcChannel *channel,
                    ConnectedDescriptor *descriptor) {
  delete channel;
  delete descriptor;
}
}  // namespace

const char RpcServer::K_CLIENT_VAR[] = "clients-connected";
const char RpcServer::K_RPC_PORT_VAR[] = "rpc-port";

RpcServer::RpcServer(ola::io::SelectServerInterface *ss,
                     RpcService *service,
                     RpcSessionHandlerInterface *session_handler,
                     const Options &options)
    : m_ss(ss),
      m_service(service),
      m_session_handler(session_handler),
      m_options(options),
      m_tcp_socket_factory(
          ola::NewCallback(this, &RpcServer::NewTCPConnection)) {
  if (m_options.export_map) {
    m_options.export_map->GetIntegerVar(K_CLIENT_VAR);
  }
}

RpcServer::~RpcServer() {
  // Take a copy since calling the close handler will cause the socket to be
  // removed from m_connected_sockets
  ClientDescriptors sockets = m_connected_sockets;
  ClientDescriptors::const_iterator iter = sockets.begin();
  for (; iter != sockets.end(); ++iter) {
    (*iter)->TransferOnClose()->Run();
  }

  if (!sockets.empty()) {
    m_ss->DrainCallbacks();
  }

  if (m_accepting_socket.get() && m_accepting_socket->ValidReadDescriptor()) {
    m_ss->RemoveReadDescriptor(m_accepting_socket.get());
  }
}

bool RpcServer::Init() {
  if (m_accepting_socket.get()) {
    return false;
  }

  auto_ptr<TCPAcceptingSocket> accepting_socket;

  if (m_options.listen_socket) {
    accepting_socket.reset(m_options.listen_socket);
    accepting_socket->SetFactory(&m_tcp_socket_factory);
  } else {
    accepting_socket.reset(
      new TCPAcceptingSocket(&m_tcp_socket_factory));

    if (!accepting_socket->Listen(
          IPV4SocketAddress(IPV4Address::Loopback(), m_options.listen_port))) {
      OLA_FATAL << "Could not listen on the RPC port " << m_options.listen_port
                << ", you probably have another instance running.";
      return false;
    }
    if (m_options.export_map) {
      m_options.export_map->GetIntegerVar(K_RPC_PORT_VAR)->Set(
          m_options.listen_port);
    }
  }

  if (!m_ss->AddReadDescriptor(accepting_socket.get())) {
    OLA_WARN << "Failed to add RPC socket to SelectServer";
    return false;
  }

  m_accepting_socket.reset(accepting_socket.release());
  return true;
}

GenericSocketAddress RpcServer::ListenAddress() {
  if (m_accepting_socket.get()) {
    return m_accepting_socket->GetLocalAddress();
  }
  return GenericSocketAddress();
}

bool RpcServer::AddClient(ConnectedDescriptor *descriptor) {
  // If RpcChannel had a pointer to the SelectServer to use, we could hand off
  // ownership of the socket here.
  RpcChannel *channel = new RpcChannel(m_service, descriptor,
                                       m_options.export_map);

  if (m_session_handler) {
    m_session_handler->NewClient(channel->Session());
  }

  channel->SetChannelCloseHandler(
      NewSingleCallback(this, &RpcServer::ChannelClosed, descriptor));

  if (m_options.export_map) {
    (*m_options.export_map->GetIntegerVar(K_CLIENT_VAR))++;
  }

  m_ss->AddReadDescriptor(descriptor);
  m_connected_sockets.insert(descriptor);

  return true;
}

void RpcServer::NewTCPConnection(TCPSocket *socket) {
  if (!socket)
    return;

  socket->SetNoDelay();
  AddClient(socket);
}

void RpcServer::ChannelClosed(ConnectedDescriptor *descriptor,
                              RpcSession *session) {
  if (m_session_handler) {
    m_session_handler->ClientRemoved(session);
  }

  if (m_options.export_map) {
    (*m_options.export_map->GetIntegerVar(K_CLIENT_VAR))--;
  }

  m_ss->RemoveReadDescriptor(descriptor);
  m_connected_sockets.erase(descriptor);

  // We're in the call stack of both the descriptor and the channel here.
  // We schedule deletion during the next run of the event loop to break out of
  // the stack.
  m_ss->Execute(
      NewSingleCallback(CleanupChannel, session->Channel(), descriptor));
}
}  // namespace rpc
}  // namespace ola
