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
 * AdvancedTCPConnector.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_ADVANCEDTCPCONNECTOR_H_
#define INCLUDE_OLA_NETWORK_ADVANCEDTCPCONNECTOR_H_

#include <math.h>

#include <ola/Callback.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/TCPConnector.h>
#include <ola/network/TCPSocketFactory.h>
#include <ola/util/Backoff.h>
#include <map>
#include <utility>

namespace ola {
namespace network {

/**
 * Manages the TCP connections to ip:ports.
 * The AdvancedTCPConnector attempts to open connections to ip:ports,
 * backing off exponentially if we can't connect.
 *
 * Limitiations:
 *  This class only supports a single connection per ip:port.
 *  This class should work fine for a small number of TCP connections (100 or
 *   so). It'll need to be re-written if we want to support 1000s.
 */
class AdvancedTCPConnector {
 public:
    AdvancedTCPConnector(ola::io::SelectServerInterface *ss,
                         TCPSocketFactoryInterface *socket_factory,
                         const ola::TimeInterval &connection_timeout);
    virtual ~AdvancedTCPConnector();

    void AddEndpoint(const IPV4SocketAddress &endpoint,
                     BackOffPolicy *backoff_policy,
                     bool paused = false);
    void RemoveEndpoint(const IPV4SocketAddress &endpoint);

    unsigned int EndpointCount() const { return m_connections.size(); }

    enum ConnectionState {
      DISCONNECTED,  // socket is disconnected
      PAUSED,  // disconnected, and we don't want to reconnect right now.
      CONNECTED,  // socket is connected
    };

    bool GetEndpointState(const IPV4SocketAddress &endpoint,
                          ConnectionState *connected,
                          unsigned int *failed_attempts) const;

    void Disconnect(const IPV4SocketAddress &endpoint, bool pause = false);
    void Resume(const IPV4SocketAddress &endpoint);

 protected:
    typedef struct {
      ConnectionState state;
      unsigned int failed_attempts;
      ola::thread::timeout_id retry_timeout;
      TCPConnector::TCPConnectionID connection_id;
      BackOffPolicy *policy;
    } ConnectionInfo;

    typedef std::pair<IPV4Address, uint16_t> IPPortPair;

    TCPSocketFactoryInterface *m_socket_factory;

    /**
     * Sub classes can override this to tune the behavior
     */
    virtual void TakeAction(const IPPortPair &key,
                            ConnectionInfo *info,
                            int fd,
                            int error);
    void ScheduleRetry(const IPPortPair &key, ConnectionInfo *info);

 private:
    ola::io::SelectServerInterface *m_ss;

    TCPConnector m_connector;
    const ola::TimeInterval m_connection_timeout;

    typedef std::map<IPPortPair, ConnectionInfo*> ConnectionMap;
    ConnectionMap m_connections;

    void RetryTimeout(IPPortPair key);
    void ConnectionResult(IPPortPair key, int fd, int error);
    void AttemptConnection(const IPPortPair &key, ConnectionInfo *state);
    void AbortConnection(ConnectionInfo *state);
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_ADVANCEDTCPCONNECTOR_H_
