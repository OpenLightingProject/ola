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
 * TCPConnector.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <errno.h>

#ifdef _WIN32
#include <Ws2tcpip.h>
#ifndef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT
#endif
#endif

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/TCPConnector.h"

namespace ola {
namespace network {

TCPConnector::TCPConnector(ola::io::SelectServerInterface *ss)
    : m_ss(ss) {
}


/**
 * Clean up
 */
TCPConnector::~TCPConnector() {
  CancelAll();
}


/**
 * Perform a non-blocking connect.
 * on_connect may be called immediately if the address is local.
 * on_failure will be called immediately if an error occurs
 * @param endpoint the IPV4SocketAddress to connect to
 * @param timeout the time to wait before declaring the connection a failure
 * @param callback the callback to run when the connection completes or fails
 * @returns the ID for this connection, or 0 if the callback has already
 * run.
 */
TCPConnector::TCPConnectionID TCPConnector::Connect(
    const IPV4SocketAddress &endpoint,
    const ola::TimeInterval &timeout,
    TCPConnectCallback *callback) {
  struct sockaddr server_address;
  if (!endpoint.ToSockAddr(&server_address, sizeof(server_address))) {
    callback->Run(-1, 0);
    return 0;
  }

  int sd = socket(endpoint.Family(), SOCK_STREAM, 0);
  if (sd < 0) {
    int error = errno;
    OLA_WARN << "socket() failed, " << strerror(error);
    callback->Run(-1, error);
    return 0;
  }

#ifdef _WIN32
  ola::io::DescriptorHandle descriptor;
  descriptor.m_handle.m_fd = sd;
  descriptor.m_type = ola::io::SOCKET_DESCRIPTOR;
  descriptor.m_event_handle = 0;
#else
  ola::io::DescriptorHandle descriptor = sd;
#endif
  ola::io::ConnectedDescriptor::SetNonBlocking(descriptor);

  int r = connect(sd, &server_address, sizeof(server_address));

  if (r) {
#ifdef _WIN32
    if (WSAGetLastError() != WSAEINPROGRESS) {
#else
    if (errno != EINPROGRESS) {
#endif
      int error = errno;
      OLA_WARN << "connect to " << endpoint << " failed, " << strerror(error);
      close(sd);
      callback->Run(-1, error);
      return 0;
    }
  } else {
    // connect returned immediately
    callback->Run(sd, 0);
    return 0;
  }

  PendingTCPConnection *connection = new PendingTCPConnection(
    this,
    endpoint.Host(),
    sd,
    callback);

  m_connections.insert(connection);

  connection->timeout_id = m_ss->RegisterSingleTimeout(
    timeout,
    ola::NewSingleCallback(this, &TCPConnector::TimeoutEvent, connection));

  m_ss->AddWriteDescriptor(connection);
  return connection;
}


/**
 * Cancel a pending TCP connection
 * @param id the TCPConnectionID
 * @return true if this connection was cancelled, false if the id wasn't valid.
 */
bool TCPConnector::Cancel(TCPConnectionID id) {
  PendingTCPConnection *connection =
    const_cast<PendingTCPConnection*>(
        reinterpret_cast<const PendingTCPConnection*>(id));
  ConnectionSet::iterator iter = m_connections.find(connection);
  if (iter == m_connections.end())
    return false;

  Timeout(iter);
  m_connections.erase(iter);
  return true;
}


/**
 * Abort all pending TCP connections
 */
void TCPConnector::CancelAll() {
  ConnectionSet::iterator iter = m_connections.begin();
  for (; iter != m_connections.end(); ++iter)
    Timeout(iter);
  m_connections.clear();
}


/**
 * Called when the socket becomes writeable
 */
void TCPConnector::SocketWritable(PendingTCPConnection *connection) {
  // cancel timeout
  m_ss->RemoveTimeout(connection->timeout_id);
  m_ss->RemoveWriteDescriptor(connection);

  // fetch the error code
#ifdef _WIN32
  int sd = connection->WriteDescriptor().m_handle.m_fd;
#else
  int sd = connection->WriteDescriptor();
#endif
  int error = 0;
  socklen_t len;
  len = sizeof(error);
#ifdef _WIN32
  int r = getsockopt(sd, SOL_SOCKET, SO_ERROR,
                     reinterpret_cast<char*>(&error), &len);
#else
  int r = getsockopt(sd, SOL_SOCKET, SO_ERROR, &error, &len);
#endif
  if (r < 0)
    error = errno;

  if (error) {
    OLA_WARN << "Failed to connect to " << connection->ip_address << " error "
      << strerror(error);
    connection->Close();
    connection->callback->Run(-1, error);
  } else {
#ifdef _WIN32
    connection->callback->Run(connection->WriteDescriptor().m_handle.m_fd, 0);
#else
    connection->callback->Run(connection->WriteDescriptor(), 0);
#endif
  }

  ConnectionSet::iterator iter = m_connections.find(connection);
  if (iter != m_connections.end())
    m_connections.erase(iter);

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



void TCPConnector::Timeout(const ConnectionSet::iterator &iter) {
  PendingTCPConnection *connection = *iter;
  m_ss->RemoveTimeout(connection->timeout_id);
  m_ss->RemoveWriteDescriptor(connection);
  connection->Close();
  TCPConnectCallback *callback = connection->callback;
  delete connection;
  callback->Run(-1, ETIMEDOUT);
}


/**
 * Called when the connect() times out.
 */
void TCPConnector::TimeoutEvent(PendingTCPConnection *connection) {
  ConnectionSet::iterator iter = m_connections.find(connection);
  if (iter == m_connections.end()) {
    OLA_FATAL <<
      "Timeout triggered but couldn't find the connection this refers to";
  }

  Timeout(iter);
  m_connections.erase(iter);
}


TCPConnector::PendingTCPConnection::PendingTCPConnection(
    TCPConnector *connector,
    const IPV4Address &ip,
    int fd,
    TCPConnectCallback *callback)
        : WriteFileDescriptor(),
          ip_address(ip),
          callback(callback),
          timeout_id(ola::thread::INVALID_TIMEOUT),
          m_connector(connector) {
#ifdef _WIN32
  m_handle.m_handle.m_fd = fd;
  m_handle.m_type = ola::io::SOCKET_DESCRIPTOR;
  m_handle.m_event_handle = 0;
#else
  m_handle = fd;
#endif
}


/**
 * Close this connection
 */
void TCPConnector::PendingTCPConnection::Close() {
#ifdef _WIN32
  close(m_handle.m_handle.m_fd);
#else
  close(m_handle);
#endif
}


/**
 * Called when the socket becomes writeable
 */
void TCPConnector::PendingTCPConnection::PerformWrite() {
  m_connector->SocketWritable(this);
}
}  // namespace network
}  // namespace ola
