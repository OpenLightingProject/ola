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
 * AdvancedTCPConnector.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/AdvancedTCPConnector.h>
#include <ola/network/TCPConnector.h>


namespace ola {
namespace network {

using std::pair;


/**
 * Create a new AdvancedTCPConnector
 * @param ss the SelectServerInterface to use for scheduling
 * @param socket_factory the factory to use for creating new sockets
 * @param connection_timeout the timeout for TCP connects
 * @param max_backoff the maximum time to wait between connects.
 */
AdvancedTCPConnector::AdvancedTCPConnector(
    ola::io::SelectServerInterface *ss,
    TCPSocketFactoryInterface *socket_factory,
    const ola::TimeInterval &connection_timeout)
    : m_socket_factory(socket_factory),
      m_ss(ss),
      m_connector(ss),
      m_connection_timeout(connection_timeout) {
}


/**
 * Cancel all outstanding connections.
 */
AdvancedTCPConnector::~AdvancedTCPConnector() {
  ConnectionMap::iterator iter = m_connections.begin();
  for (; iter != m_connections.end(); ++iter) {
    AbortConnection(iter->second);
    delete iter->second;
  }
  m_connections.clear();
}


/**
 * Add a remote host. This will trigger the connection process to start.
 * If the ip:port already exists this won't do anything.
 * When the connection is successfull the on_connect callback will be run, and
 * ownership of the TCPSocket object is transferred.
 * @param endpoint the IPV4SocketAddress to connect to.
 * @param backoff_policy the BackOffPolicy to use for this connection.
 * @param paused true if we don't want to immediately connect to this peer.
 */
void AdvancedTCPConnector::AddEndpoint(const IPV4SocketAddress &endpoint,
                                       BackOffPolicy *backoff_policy,
                                       bool paused) {
  IPPortPair key(endpoint.Host(), endpoint.Port());
  ConnectionMap::iterator iter = m_connections.find(key);
  if (iter != m_connections.end())
    return;

  // new ip:port
  ConnectionInfo *state = new ConnectionInfo;
  state->state = paused ? PAUSED : DISCONNECTED;
  state->failed_attempts = 0;
  state->retry_timeout = ola::thread::INVALID_TIMEOUT;
  state->connection_id = 0;
  state->policy = backoff_policy;

  m_connections[key] = state;

  if (!paused)
    AttemptConnection(key, state);
}


/**
 * Remove a ip:port from the connection manager. This won't close the
 * connection.
 * @param endpoint the IPV4SocketAddress to remove.
 */
void AdvancedTCPConnector::RemoveEndpoint(const IPV4SocketAddress &endpoint) {
  IPPortPair key(endpoint.Host(), endpoint.Port());
  ConnectionMap::iterator iter = m_connections.find(key);
  if (iter == m_connections.end())
    return;

  AbortConnection(iter->second);
  delete iter->second;
  m_connections.erase(iter);
}


/**
 * Get the state & number of failed_attempts for an endpoint
 * @param endpoint the IPV4SocketAddress to get the state of.
 * @returns true if this endpoint was found, false otherwise.
 */
bool AdvancedTCPConnector::GetEndpointState(
    const IPV4SocketAddress &endpoint,
    ConnectionState *connected,
    unsigned int *failed_attempts) const {
  IPPortPair key(endpoint.Host(), endpoint.Port());
  ConnectionMap::const_iterator iter = m_connections.find(key);
  if (iter == m_connections.end())
    return false;

  *connected = iter->second->state;
  *failed_attempts = iter->second->failed_attempts;
  return true;
}


/**
 * Mark a host as disconnected.
 * @param endpoint the IPV4SocketAddress to mark as disconnected.
 * @param pause if true, don't immediately try to reconnect.
 */
void AdvancedTCPConnector::Disconnect(const IPV4SocketAddress &endpoint,
                                      bool pause) {
  IPPortPair key(endpoint.Host(), endpoint.Port());
  ConnectionMap::iterator iter = m_connections.find(key);
  if (iter == m_connections.end())
    return;

  if (iter->second->state != CONNECTED)
    return;

  iter->second->failed_attempts = 0;

  if (pause) {
    iter->second->state = PAUSED;
  } else {
    // schedule a retry as if this endpoint failed once
    iter->second->state = DISCONNECTED;
    iter->second->retry_timeout = m_ss->RegisterSingleTimeout(
        iter->second->policy->BackOffTime(1),
        ola::NewSingleCallback(
          this,
          &AdvancedTCPConnector::RetryTimeout,
          iter->first));
  }
}


/**
 * Resume trying to connect to a ip:port pair.
 */
void AdvancedTCPConnector::Resume(const IPV4SocketAddress &endpoint) {
  IPPortPair key(endpoint.Host(), endpoint.Port());
  ConnectionMap::iterator iter = m_connections.find(key);
  if (iter == m_connections.end())
    return;

  if (iter->second->state == PAUSED) {
    iter->second->state = DISCONNECTED;
    AttemptConnection(iter->first, iter->second);
  }
}


/**
 * Decide what to do when a connection fails, completes or times out.
 */
void AdvancedTCPConnector::TakeAction(const IPPortPair &key,
                                      ConnectionInfo *info,
                                      int fd,
                                      int) {
  if (fd != -1) {
    // ok
    info->state = CONNECTED;
    m_socket_factory->NewTCPSocket(fd);
  } else {
    // error
    info->failed_attempts++;
    ScheduleRetry(key, info);
  }
}


/**
 * Schedule the re-try attempt for this connection
 */
void AdvancedTCPConnector::ScheduleRetry(const IPPortPair &key,
                                         ConnectionInfo *info) {
  info->retry_timeout = m_ss->RegisterSingleTimeout(
      info->policy->BackOffTime(info->failed_attempts),
      ola::NewSingleCallback(
        this,
        &AdvancedTCPConnector::RetryTimeout,
        key));
}


/**
 * Called when it's time to retry
 */
void AdvancedTCPConnector::RetryTimeout(IPPortPair key) {
  ConnectionMap::iterator iter = m_connections.find(key);
  if (iter == m_connections.end()) {
    OLA_FATAL << "Re-connect timer expired but unable to find state entry for "
      << key.first << ":" << key.second;
    return;
  }
  iter->second->retry_timeout = ola::thread::INVALID_TIMEOUT;
  AttemptConnection(key, iter->second);
}


/**
 * Called by the TCPConnector when a connection is ready or it times out.
 */
void AdvancedTCPConnector::ConnectionResult(IPPortPair key,
                                            int fd,
                                            int error) {
  if (fd != -1) {
    OLA_INFO << "TCP Connection established to " << key.first << ":" <<
      key.second;
  }

  ConnectionMap::iterator iter = m_connections.find(key);
  if (iter == m_connections.end()) {
    OLA_FATAL << "Unable to find state for " << key.first << ":" <<
      key.second << ", leaking sockets";
    return;
  }

  iter->second->connection_id = 0;
  TakeAction(iter->first, iter->second, fd, error);
}


/**
 * Initiate a connection to this ip:port pair
 */
void AdvancedTCPConnector::AttemptConnection(const IPPortPair &key,
                                             ConnectionInfo *state) {
  state->connection_id = m_connector.Connect(
      IPV4SocketAddress(key.first, key.second),
      m_connection_timeout,
      ola::NewSingleCallback(this,
                             &AdvancedTCPConnector::ConnectionResult,
                             key));
}


/**
 * Abort and clean up a pending connection
 * @param state the ConnectionInfo to cleanup.
 */
void AdvancedTCPConnector::AbortConnection(ConnectionInfo *state) {
  if (state->connection_id) {
    if (!m_connector.Cancel(state->connection_id))
      OLA_WARN << "Failed to cancel connection " << state->connection_id;
  }
  if (state->retry_timeout != ola::thread::INVALID_TIMEOUT)
    m_ss->RemoveTimeout(state->retry_timeout);
}
}  // network
}  // ola
