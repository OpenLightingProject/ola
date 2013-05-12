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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SLPClient.h
 * The API to talk to the SLP Server.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_SLP_SLPCLIENT_H_
#define INCLUDE_OLA_SLP_SLPCLIENT_H_

#include <ola/Callback.h>
#include <ola/OlaClientWrapper.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>
#include <ola/slp/Base.h>
#include <ola/slp/URLEntry.h>

#include <memory>
#include <string>
#include <vector>

namespace ola {
namespace slp {

using std::auto_ptr;
using std::string;
using ola::network::IPV4SocketAddress;
using ola::network::IPV4Address;

// Information about the server
class ServerInfo {
  public:
    bool da_enabled;
    uint16_t port;
    vector<string> scopes;

    ServerInfo() : da_enabled(false), port(0) {}
};

// Used to communicate with the local SLP server.
class SLPClient {
  public:
    explicit SLPClient(ola::io::ConnectedDescriptor *descriptor);
    ~SLPClient();

    bool Setup();
    bool Stop();

    /**
     * Register a service
     */
    bool RegisterService(
        const vector<string> &scopes,
        const string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const string&, uint16_t> *callback);

    /**
     * Register a service that persists beyond the lifetime of this client.
     */
    bool RegisterPersistentService(
        const vector<string> &scopes,
        const string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const string&, uint16_t> *callback);

    /**
     * DeRegister a service
     */
    bool DeRegisterService(
        const vector<string> &scopes,
        const string &service,
        SingleUseCallback2<void, const string&, uint16_t> *callback);

    /**
     * Find a service.
     */
    bool FindService(
        const vector<string> &scopes,
        const string &service,
        SingleUseCallback2<void, const string&,
                           const vector<URLEntry> &> *callback);

    /**
     * Get info about the server
     */
    bool GetServerInfo(
        SingleUseCallback2<void, const string&, const ServerInfo&> *callback);

  private:
    SLPClient(const SLPClient&);
    SLPClient operator=(const SLPClient&);

    auto_ptr<class SLPClientCore> m_core;
};


class SLPClientWrapper: public BaseClientWrapper {
  public:
    SLPClientWrapper() : BaseClientWrapper() {}

    SLPClient *GetClient() const { return m_client.get(); }

  private:
    auto_ptr<SLPClient> m_client;

    void CreateClient() {
      if (!m_client.get())
        m_client.reset(new SLPClient(m_socket.get()));
    }

    bool StartupClient() { return m_client->Setup(); }

    void InitSocket() {
      m_socket.reset(TCPSocket::Connect(
            IPV4SocketAddress(IPV4Address::Loopback(), OLA_SLP_DEFAULT_PORT)));
    }
};
}  // namespace slp
}  // namespace ola
#endif  // INCLUDE_OLA_SLP_SLPCLIENT_H_
