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
 * Copyright (C) 2014 Simon Newton
 */
//! [UDP Server] NOLINT(whitespace/comments)
#include <ola/Logging.h>
#include <ola/base/Array.h>
#include <ola/io/SelectServer.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::UDPSocket;

// The port to listen on.
static const unsigned int PORT = 12345;

// Called when a UDP datagram arrives.
void ReceiveMessage(UDPSocket *socket) {
  ola::network::IPV4SocketAddress client;

  uint8_t data[1500];
  ssize_t data_size = arraysize(data);
  if (socket->RecvFrom(data, &data_size, &client)) {
    OLA_INFO << "Received " << data_size << " bytes from " << client;
  } else {
    OLA_WARN << "Recv failure";
  }
}

int main() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  IPV4SocketAddress listen_address(IPV4Address::WildCard(), PORT);

  UDPSocket udp_socket;
  if (!udp_socket.Init()) {
    return -1;
  }
  if (!udp_socket.Bind(listen_address)) {
    return -1;
  }
  udp_socket.SetOnData(ola::NewCallback(&ReceiveMessage, &udp_socket));

  ola::io::SelectServer ss;
  ss.AddReadDescriptor(&udp_socket);
  ss.Run();
}
//! [UDP Server] NOLINT(whitespace/comments)
