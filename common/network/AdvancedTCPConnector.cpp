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
#include <ola/network/AdvancedTCPConnector.h>
#include <ola/network/TCPConnector.h>
#include <ola/thread/SchedulerInterface.h>


namespace ola {
namespace network {

using std::pair;


/**
 * Create a new AdvancedTCPConnector
 * @param ss the SelectServerInterface to use for scheduling
 * @param on_connect the Callback to execute when a connection is successfull
 * @param connection_timeout the timeout for TCP connects
 * @param max_backoff the maximum time to wait between connects.
 */
AdvancedTCPConnector::AdvancedTCPConnector(
    ola::network::SelectServerInterface *ss,
    OnConnect *on_connect,
    const ola::TimeInterval &connection_timeout)
    : m_ss(ss),
      m_on_connect(on_connect),
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

  if (m_on_connect)
    delete m_on_connect;
}


/**
 * Add a remote host. This will trigger the connection process to start.
 * If the ip:port already exists this won't do anything.
 * When the connection is successfull the on_connect callback will be run, and
 * ownership of the TcpSocket object is transferred.
 * @param ip_address the IP of the node to connect to
 * @param port the port to connect to
 * @param backoff_policy the BackOffPolicy to use for this connection.
 * @param paused true if we don't want to immediately connect to this peer.
 */
void AdvancedTCPConnector::AddEndpoint(const IPV4Address &ip_address,
                                       uint16_t port,
                                       BackOffPolicy *backoff_policy,
                                       bool paused) {
  IPPortPair key(ip_address, port);
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
 * @param ip_address the IP of the host to remove
 * @param port the port to remove
 */
void AdvancedTCPConnector::RemoveEndpoint(const IPV4Address &ip_address,
                                          uint16_t port) {
  IPPortPair key(ip_address, port);
  ConnectionMap::iterator iter = m_connections.find(key);
  if (iter == m_connections.end())
    return;

  AbortConnection(iter->second);
  delete iter->second;
  m_connections.erase(iter);
}


/**
 * Get the state & number of failed_attempts for an endpoint
 * @param ip_address the IP of the host to remove
 * @param port the port to remove
 * @returns true if this endpoint was found, false otherwise.
 */
bool AdvancedTCPConnector::GetEndpointState(
    const IPV4Address &ip_address,
    uint16_t port,
    ConnectionState *connected,
    unsigned int *failed_attempts) const {
  IPPortPair key(ip_address, port);
  ConnectionMap::const_iterator iter = m_connections.find(key);
  if (iter == m_connections.end())
    return false;

  *connected = iter->second->state;
  *failed_attempts = iter->second->failed_attempts;
  return true;
}


/**
 * Mark a host as disconnected.
 * @param ip_address the IP of the host that is now disconnected.
 * @param port the port that is now distributed
 * @param pause if true, don't immediately try to reconnect.
 */
void AdvancedTCPConnector::Disconnect(const IPV4Address &ip_address,
                                      uint16_t port,
                                      bool pause) {
  IPPortPair key(ip_address, port);
  ConnectionMap::iterator iter = m_connections.find(key);
  if (iter == m_connections.end())
    return;

  if (iter->second->state != CONNECTED)
    return;

  iter->second->failed_attempts = 0;

  if (pause) {
    iter->second->state = PAUSED;
  } else {
    // We try to re-connect immediately. If we have a large number of
    // connections we should stagger the re-connects. For now we assume we'll
    // have < 100  connections, so it's not a problem.
    iter->second->state = DISCONNECTED;
    AttemptConnection(iter->first, iter->second);
  }
}


/**
 * Resume trying to connect to a ip:port pair.
 */
void AdvancedTCPConnector::Resume(const IPV4Address &ip_address,
                                  uint16_t port) {
  IPPortPair key(ip_address, port);
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
                                      TcpSocket *socket,
                                      int) {
  if (socket) {
    // ok
    info->state = CONNECTED;
    m_on_connect->Run(key.first, key.second, socket);
  } else {
    // error
    info->failed_attempts++;
    info->retry_timeout = m_ss->RegisterSingleTimeout(
        info->policy->BackOffTime(info->failed_attempts),
        ola::NewSingleCallback(this, &AdvancedTCPConnector::RetryTimeout,
          key));
  }
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
                                            TcpSocket *socket,
                                            int error) {
  if (socket) {
    OLA_INFO << "TCP Connection established to " << key.first << ":" <<
      key.second;
  }

  ConnectionMap::iterator iter = m_connections.find(key);
  if (iter == m_connections.end()) {
    OLA_FATAL << "Unable to find state for " << key.first << ":" <<
      key.second;
    if (socket) {
      socket->Close();
      delete socket;
    }
    return;
  }

  iter->second->connection_id = 0;
  TakeAction(iter->first, iter->second, socket, error);
}


/**
 * Initiate a connection to this ip:port pair
 */
void AdvancedTCPConnector::AttemptConnection(const IPPortPair &key,
                                             ConnectionInfo *state) {
  state->connection_id = m_connector.Connect(
      key.first,
      key.second,
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
