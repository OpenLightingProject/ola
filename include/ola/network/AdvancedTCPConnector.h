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
#include <map>


namespace ola {
namespace network {


/**
 * The Interface for BackOff Policies.
 */
class BackOffPolicy {
  public:
    BackOffPolicy() {}
    virtual ~BackOffPolicy() {}

    /**
     * Failed attempts is the number of unsuccessfull connection attempts since
     * the last successful connection. Will be >= 1.
     */
    virtual TimeInterval BackOffTime(unsigned int failed_attempts) const = 0;
};


/**
 * Constant time back off polcy
 */
class ConstantBackoffPolicy: public BackOffPolicy {
  public:
    explicit ConstantBackoffPolicy(const TimeInterval &duration)
        : m_duration(duration) {
    }

    TimeInterval BackOffTime(unsigned int) const {
      return m_duration;
    }

  private:
    const TimeInterval m_duration;
};


/**
 * A backoff policy which is:
 *   t = failed_attempts * duration
 */
class LinearBackoffPolicy: public BackOffPolicy {
  public:
    LinearBackoffPolicy(const TimeInterval &duration, const TimeInterval &max)
        : m_duration(duration),
          m_max(max) {
    }

    TimeInterval BackOffTime(unsigned int failed_attempts) const {
      TimeInterval interval = m_duration * failed_attempts;
      if (interval > m_max)
        interval = m_max;
      return interval;
    }

  private:
    const TimeInterval m_duration;
    const TimeInterval m_max;
};


/**
 * A backoff policy which is:
 *   t = initial * 2 ^ failed_attempts
 */
class ExponentialBackoffPolicy: public BackOffPolicy {
  public:
    ExponentialBackoffPolicy(const TimeInterval &initial,
                             const TimeInterval &max)
        : m_initial(initial),
          m_max(max) {
    }

    TimeInterval BackOffTime(unsigned int failed_attempts) const {
      TimeInterval interval = (
          m_initial * static_cast<int>(::pow(2, failed_attempts - 1)));
      if (interval > m_max)
        interval = m_max;
      return interval;
    }

  private:
    const TimeInterval m_initial;
    const TimeInterval m_max;
};


/**
 * Manages the TCP connections to ip:ports.
 * The AdvancedTCPConnector failed_attempts to open connections to ip:ports,
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

    void AddEndpoint(const IPV4Address &ip_address,
                     uint16_t port,
                     BackOffPolicy *backoff_policy,
                     bool paused = false);
    void RemoveEndpoint(const IPV4Address &ip_address, uint16_t port);

    unsigned int EndpointCount() const { return m_connections.size(); }

    enum ConnectionState {
      DISCONNECTED,  // socket is disconnected
      PAUSED,  // disconnected, and we don't want to reconnect right now.
      CONNECTED,  // socket is connected
    };

    bool GetEndpointState(const IPV4Address &ip_address,
                          uint16_t port,
                          ConnectionState *connected,
                          unsigned int *failed_attempts) const;

    void Disconnect(const IPV4Address &ip_address,
                    uint16_t port,
                    bool pause = false);
    void Resume(const IPV4Address &ip_address, uint16_t port);

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
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_ADVANCEDTCPCONNECTOR_H_
