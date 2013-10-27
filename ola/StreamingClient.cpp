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

#include <ola/AutoStart.h>
#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>
#include <ola/StreamingClient.h>
#include "common/protocol/Ola.pb.h"
#include "common/protocol/OlaService.pb.h"
#include "common/rpc/StreamRpcChannel.h"

namespace ola {

using ola::rpc::StreamRpcChannel;
using ola::proto::OlaServerService_Stub;

StreamingClient::StreamingClient(bool auto_start)
    : m_auto_start(auto_start),
      m_server_port(OLA_DEFAULT_PORT),
      m_socket(NULL),
      m_ss(NULL),
      m_channel(NULL),
      m_stub(NULL),
      m_socket_closed(false) {
}

StreamingClient::StreamingClient(const Options &options)
    : m_auto_start(options.auto_start),
      m_server_port(options.server_port),
      m_socket(NULL),
      m_ss(NULL),
      m_channel(NULL),
      m_stub(NULL),
      m_socket_closed(false) {
}

StreamingClient::~StreamingClient() {
  Stop();
}

bool StreamingClient::Setup() {
  if (m_socket || m_channel || m_stub)
    return false;

  if (m_auto_start)
    m_socket = ola::client::ConnectToServer(m_server_port);
  else
    m_socket = TCPSocket::Connect(
      ola::network::IPV4SocketAddress(ola::network::IPV4Address::Loopback(),
                                      m_server_port));

  if (!m_socket)
    return false;

  m_ss = new SelectServer();
  m_ss->AddReadDescriptor(m_socket);

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

  m_channel->SetChannelCloseHandler(
      NewSingleCallback(this, &StreamingClient::ChannelClosed));

  return true;
}

void StreamingClient::Stop() {
  if (m_stub)
    delete m_stub;

  if (m_channel)
    delete m_channel;

  if (m_ss)
    delete m_ss;

  if (m_socket)
    delete m_socket;

  m_channel = NULL;
  m_socket = NULL;
  m_ss = NULL;
  m_stub = NULL;
}

bool StreamingClient::SendDmx(unsigned int universe,
                              const DmxBuffer &data) {
  if (!m_stub || !m_socket->ValidReadDescriptor())
    return false;

  // We select() on the fd here to see if the remove end has closed the
  // connection. We could skip this and rely on the EPIPE delivered by the
  // write() below, but that introduces a race condition in the unittests.
  m_socket_closed = false;
  m_ss->RunOnce(0, 0);

  if (m_socket_closed) {
    Stop();
    return false;
  }

  ola::proto::DmxData request;
  request.set_universe(universe);
  request.set_data(data.Get());
  m_stub->StreamDmxData(NULL, &request, NULL, NULL);

  if (m_socket_closed) {
    Stop();
    return false;
  }
  return true;
}

void StreamingClient::ChannelClosed() {
  m_socket_closed = true;
  OLA_WARN << "The RPC socket has been closed, this is more than likely due"
    << " to a framing error, perhaps you're sending too fast?";
}
}  // namespace ola
