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
 * TCPConnector.h.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_TCPCONNECTOR_H_
#define INCLUDE_OLA_NETWORK_TCPCONNECTOR_H_

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/TCPSocket.h>
#include <set>


namespace ola {
namespace network {

/**
 * An class which manages non-blocking TCP connects.
 */
class TCPConnector {
  public:
    explicit TCPConnector(ola::io::SelectServerInterface *ss);
    ~TCPConnector();

    typedef ola::SingleUseCallback2<void, int, int> TCPConnectCallback;
    typedef const void* TCPConnectionID;

    TCPConnectionID Connect(const IPV4SocketAddress &endpoint,
                            const ola::TimeInterval &timeout,
                            TCPConnectCallback *callback);

    bool Cancel(TCPConnectionID id);
    void CancelAll();
    unsigned int ConnectionsPending() const { return m_connections.size(); }

  private:
    /**
     * A TCP socket waiting to connect.
     */
    class PendingTCPConnection: public ola::io::WriteFileDescriptor {
      public:
        PendingTCPConnection(TCPConnector *connector,
                             const IPV4Address &ip,
                             int fd,
                             TCPConnectCallback *callback)
            : WriteFileDescriptor(),
              ip_address(ip),
              callback(callback),
              timeout_id(ola::thread::INVALID_TIMEOUT),
              m_connector(connector),
              m_fd(fd) {
        }

        int WriteDescriptor() const { return m_fd; }

        void PerformWrite();
        void Close();

        const IPV4Address ip_address;
        TCPConnectCallback *callback;
        ola::thread::timeout_id timeout_id;

      private:
        TCPConnector *m_connector;
        int m_fd;
    };

    typedef std::set<PendingTCPConnection*> ConnectionSet;

    ola::io::SelectServerInterface *m_ss;
    ConnectionSet m_connections;


    void SocketWritable(PendingTCPConnection *connection);
    void FreePendingConnection(PendingTCPConnection *connection);
    void Timeout(const ConnectionSet::iterator &iter);
    void TimeoutEvent(PendingTCPConnection *connection);
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_TCPCONNECTOR_H_
