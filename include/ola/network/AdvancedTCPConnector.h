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
 * AdvancedTCPConnector.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_ADVANCEDTCPCONNECTOR_H_
#define INCLUDE_OLA_NETWORK_ADVANCEDTCPCONNECTOR_H_

#include <math.h>

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/TCPConnector.h>
#include <ola/network/TCPSocketFactory.h>
#include <ola/util/Backoff.h>
#include <map>
#include <utility>

namespace ola {
namespace network {

/**
 * @brief Attempts to open a TCP connection until a failure limit is reached.
 *
 * The AdvancedTCPConnector attempts to open connections to a endpoint. If
 * the connection fails it will retry according to a given BackOffPolicy.
 *
 * Limitations:
 *  - This class only supports a single connection per IP:Port.
 *  - This class should work fine for a small number of TCP connections (100 or
 *    so). It'll need to be re-written if we want to support 1000s.
 */
class AdvancedTCPConnector {
 public:
  /**
   * @brief Create a new AdvancedTCPConnector
   * @param ss the SelectServerInterface to use for scheduling
   * @param socket_factory the factory to use for creating new sockets
   * @param connection_timeout the timeout for TCP connects.
   */
  AdvancedTCPConnector(ola::io::SelectServerInterface *ss,
                       TCPSocketFactoryInterface *socket_factory,
                       const ola::TimeInterval &connection_timeout);

  ~AdvancedTCPConnector();

  /**
   * @brief Add an endpoint to manage a connection to.
   *
   * If the IP:Port already exists this won't do anything.
   * When the connection is successful the on_connect callback will be run, and
   * ownership of the TCPSocket object is transferred.

   * @param endpoint the IPV4SocketAddress to connect to.
   * @param backoff_policy the BackOffPolicy to use for this connection.
   * @param paused true if we don't want to immediately connect to this peer.
   */
  void AddEndpoint(const IPV4SocketAddress &endpoint,
                   BackOffPolicy *backoff_policy,
                   bool paused = false);

  /**
   * @brief Remove a IP:Port from the connection manager. This won't close the
   * connection if it's already established.
   * @param endpoint the IPV4SocketAddress to remove.
   */
  void RemoveEndpoint(const IPV4SocketAddress &endpoint);

  /**
   * @brief Return the number of connections tracked by this connector.
   */
  unsigned int EndpointCount() const { return static_cast<unsigned int>(m_connections.size()); }

  /**
   * @brief The state of a connection.
   */
  enum ConnectionState {
    DISCONNECTED,  /**< The socket is disconnected */
    PAUSED,  /**< The socket is disconnected, and will not be retried. */
    CONNECTED,  /**< The socket is connected. */
  };

  /**
   * @brief Get the state & number of failed_attempts for an endpoint
   * @param endpoint the IPV4SocketAddress to get the state of.
   * @param[out] connected the connection state for this endpoint.
   * @param[out] failed_attempts the number of failed connects for this
   * endpoint.
   * @returns true if this endpoint was found, false otherwise.
   */
  bool GetEndpointState(const IPV4SocketAddress &endpoint,
                        ConnectionState *connected,
                        unsigned int *failed_attempts) const;

  /**
   * @brief Mark an endpoint as disconnected.
   * @param endpoint the IPV4SocketAddress to mark as disconnected.
   * @param pause if true, don't immediately try to reconnect.
   */
  void Disconnect(const IPV4SocketAddress &endpoint, bool pause = false);

  /**
   * @brief Resume trying to connect to a ip:port pair.
   * @param endpoint the IPV4SocketAddress to resume connecting for.
   */
  void Resume(const IPV4SocketAddress &endpoint);

 private:
  typedef struct {
    ConnectionState state;
    unsigned int failed_attempts;
    ola::thread::timeout_id retry_timeout;
    TCPConnector::TCPConnectionID connection_id;
    BackOffPolicy *policy;
    bool reconnect;
  } ConnectionInfo;

  typedef std::pair<IPV4Address, uint16_t> IPPortPair;
  typedef std::map<IPPortPair, ConnectionInfo*> ConnectionMap;

  TCPSocketFactoryInterface *m_socket_factory;
  ola::io::SelectServerInterface *m_ss;
  TCPConnector m_connector;
  const ola::TimeInterval m_connection_timeout;
  ConnectionMap m_connections;

  void ScheduleRetry(const IPPortPair &key, ConnectionInfo *info);
  void RetryTimeout(IPPortPair key);
  void ConnectionResult(IPPortPair key, int fd, int error);
  void AttemptConnection(const IPPortPair &key, ConnectionInfo *state);
  void AbortConnection(ConnectionInfo *state);

  DISALLOW_COPY_AND_ASSIGN(AdvancedTCPConnector);
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_ADVANCEDTCPCONNECTOR_H_
