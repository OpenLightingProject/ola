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
 * TCPConnector.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_TCPCONNECTOR_H_
#define INCLUDE_OLA_NETWORK_TCPCONNECTOR_H_

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/base/Macro.h>
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/TCPSocket.h>
#include <set>
#include <vector>

namespace ola {
namespace network {

/**
 * @brief An class which manages non-blocking TCP connects.
 */
class TCPConnector {
 public:
  /**
   * @brief The callback run when a TCP connection request completes.
   *
   * The first argument passed to the callback is the FD or -1 if the connect()
   * request failed. If the request failed, the second argument is errno.
   */
  typedef ola::SingleUseCallback2<void, int, int> TCPConnectCallback;

  /**
   * @brief The TCPConnectionID.
   *
   * This can be used to cancel a pending TCP connection
   */
  typedef const void* TCPConnectionID;

  /**
   * @brief Create a new TCPConnector.
   * @param ss The SelectServerInterface to use
   */
  explicit TCPConnector(ola::io::SelectServerInterface *ss);

  ~TCPConnector();

  /**
   * @brief Perform a non-blocking TCP connect.
   *
   * The callback may be run from within this method. Some platforms like *BSD
   * won't return EINPROGRESS if the address is local.
   *
   * @param endpoint the IPV4SocketAddress to connect to
   * @param timeout the time to wait before declaring the connection a failure.
   * @param callback the TCPConnectCallback to run when the connection
   *   completes.
   * @returns the TCPConnectionID for this connection, or 0 if the callback has
   *   already run.
   */
  TCPConnectionID Connect(const IPV4SocketAddress &endpoint,
                          const ola::TimeInterval &timeout,
                          TCPConnectCallback *callback);

  /**
   * @brief Cancel a pending TCP connection.
   *
   * Cancelling a connection causes the callback to be run with ETIMEDOUT.
   *
   * @param id the id of the TCP connection to cancel
   * @returns true if the connection was canceled. False if the TCPConnectionID
   *   wasn't found.
   */
  bool Cancel(TCPConnectionID id);

  /**
   * @brief Cancel all pending TCP connections.
   */
  void CancelAll();

  /**
   * @brief Return the number of pending connections
   */
  unsigned int ConnectionsPending() const { return static_cast<unsigned int>(m_connections.size()); }

  /**
   * @brief Called when the TCP socket connects.
   * @param connection the connection that is now connected.
   */
  void SocketWritable(class PendingTCPConnection *connection);

 private:
  typedef std::set<class PendingTCPConnection*> ConnectionSet;

  ola::io::SelectServerInterface *m_ss;
  ConnectionSet m_connections;

  void FreePendingConnection(class PendingTCPConnection *connection);
  void Timeout(const ConnectionSet::iterator &iter);
  void TimeoutEvent(class PendingTCPConnection *connection);
  void CleanUpOrphans();

  DISALLOW_COPY_AND_ASSIGN(TCPConnector);
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_TCPCONNECTOR_H_
