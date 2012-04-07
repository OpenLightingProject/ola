/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * TCPConnector.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <errno.h>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/TCPConnector.h"

namespace ola {
namespace network {

TCPConnector::TCPConnector(SelectServerInterface *ss)
    : m_ss(ss) {
}


/**
 * Perform a non-blocking connect.
 * on_connect may be called immediately if the address is local.
 * on_failure will be called immediately if an error occurs
 * @param ip the IP address to connect to
 * @param port the port to connect to
 * @param timeout the time to wait before declaring the connection a failure
 * @param callback the callback to run when the connection completes or fails
 */
void TCPConnector::Connect(const IPV4Address &ip_address,
                           uint16_t port,
                           const ola::TimeInterval &timeout,
                           TCPConnectCallback *callback) {
  struct sockaddr_in server_address;
  socklen_t length = sizeof(server_address);

  int sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    int error = errno;
    OLA_WARN << "socket() failed, " << strerror(error);
    callback->Run(NULL, error);
    return;
  }

  // setup
  server_address.sin_family = AF_INET;
  server_address.sin_port = HostToNetwork(port);
  server_address.sin_addr.s_addr = ip_address.AsInt();

  ConnectedDescriptor::SetNonBlocking(sd);

  int r = connect(sd, (struct sockaddr*) &server_address, length);

  if (r) {
    if (errno != EINPROGRESS) {
      int error = errno;
      OLA_WARN << "connect to " << ip_address << ":" << port << " failed, "
        << strerror(error);
      callback->Run(NULL, error);
      return;
    }
  } else {
    // connect returned immediately
    callback->Run(new TcpSocket(sd), 0);
    return;
  }

  PendingTCPConnection *connection = new PendingTCPConnection(
    this,
    ip_address,
    sd,
    callback);

  connection->timeout_id = m_ss->RegisterSingleTimeout(
    timeout,
    ola::NewSingleCallback(this, &TCPConnector::TimeoutEvent, connection));

  m_ss->AddWriteDescriptor(connection);
}


/**
 * Called when the socket becomes writeable
 */
void TCPConnector::SocketWritable(PendingTCPConnection *connection) {
  // cancel timeout
  m_ss->RemoveTimeout(connection->timeout_id);
  m_ss->RemoveWriteDescriptor(connection);

  // fetch the error code
  int sd = connection->WriteDescriptor();
  int error = 0;
  socklen_t len;
  len = sizeof(error);
  int r = getsockopt(sd, SOL_SOCKET, SO_ERROR, &error, &len);
  if (r < 0)
    error = errno;

  if (error) {
    OLA_WARN << "Failed to connect to " << connection->ip_address << " error "
      << strerror(error);
    connection->Close();
    connection->callback->Run(NULL, error);
  } else {
    connection->callback->Run(new TcpSocket(connection->WriteDescriptor()), 0);
  }

  // we're already within the PendingTCPConnection's call stack here
  // schedule the deletion to run later
  m_ss->Execute(
    ola::NewSingleCallback(this,
                           &TCPConnector::FreePendingConnection,
                           connection));
}


/**
 * Delete this pending connection
 */
void TCPConnector::FreePendingConnection(PendingTCPConnection *connection) {
  delete connection;
}


/**
 * Called when the connect() times out.
 */
void TCPConnector::TimeoutEvent(PendingTCPConnection *connection) {
  m_ss->RemoveWriteDescriptor(connection);
  connection->Close();
  TCPConnectCallback *callback = connection->callback;
  delete connection;
  callback->Run(NULL, ETIMEDOUT);
}


/**
 * Close this connection
 */
void TCPConnector::PendingTCPConnection::Close() {
  close(m_fd);
}


/**
 * Called when the socket becomes writeable
 */
void TCPConnector::PendingTCPConnection::PerformWrite() {
  m_connector->SocketWritable(this);
}
}  // network
}  // ola
