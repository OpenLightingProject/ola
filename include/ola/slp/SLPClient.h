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
#include <ola/base/Macro.h>
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


// Information about the server
class ServerInfo {
 public:
    bool da_enabled;
    uint16_t port;
    std::vector<std::string> scopes;

    ServerInfo() : da_enabled(false), port(0) {}
};

// Used to communicate with the local SLP server.
class SLPClient {
 public:
    explicit SLPClient(ola::io::ConnectedDescriptor *descriptor);
    ~SLPClient();

    bool Setup();
    bool Stop();

    void SetCloseHandler(ola::SingleUseCallback0<void> *callback);

    /**
     * Register a service
     */
    bool RegisterService(
        const std::vector<std::string> &scopes,
        const std::string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const std::string&, uint16_t> *callback);

    /**
     * Register a service that persists beyond the lifetime of this client.
     */
    bool RegisterPersistentService(
        const std::vector<std::string> &scopes,
        const std::string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const std::string&, uint16_t> *callback);

    /**
     * DeRegister a service
     */
    bool DeRegisterService(
        const std::vector<std::string> &scopes,
        const std::string &service,
        SingleUseCallback2<void, const std::string&, uint16_t> *callback);

    /**
     * Find a service.
     */
    bool FindService(
        const std::vector<std::string> &scopes,
        const std::string &service,
        SingleUseCallback2<void, const std::string&,
                           const std::vector<URLEntry> &> *callback);

    /**
     * Get info about the server
     */
    bool GetServerInfo(
        SingleUseCallback2<void,
                           const std::string&,
                           const ServerInfo&> *callback);

 private:
    std::auto_ptr<class SLPClientCore> m_core;

    DISALLOW_COPY_AND_ASSIGN(SLPClient);
};


class SLPClientWrapper: public ola::client::BaseClientWrapper {
 public:
    SLPClientWrapper() : BaseClientWrapper() {}

    SLPClient *GetClient() const { return m_client.get(); }

 private:
    std::auto_ptr<SLPClient> m_client;

    void CreateClient() {
      if (!m_client.get()) {
        m_client.reset(new SLPClient(m_socket.get()));
      }
    }

    bool StartupClient() {
      bool ok = m_client->Setup();
      m_client->SetCloseHandler(
        ola::NewSingleCallback(static_cast<BaseClientWrapper*>(this),
                               &BaseClientWrapper::SocketClosed));
      return ok;
    }

    void InitSocket() {
      m_socket.reset(ola::network::TCPSocket::Connect(
            ola::network::IPV4SocketAddress(
                ola::network::IPV4Address::Loopback(),
                OLA_SLP_DEFAULT_PORT)));
    }

    DISALLOW_COPY_AND_ASSIGN(SLPClientWrapper);
};
}  // namespace slp
}  // namespace ola
#endif  // INCLUDE_OLA_SLP_SLPCLIENT_H_
