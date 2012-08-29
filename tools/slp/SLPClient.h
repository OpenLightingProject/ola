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
 * SLPClient.h
 * The API to talk to the SLP Server.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPCLIENT_H_
#define TOOLS_SLP_SLPCLIENT_H_

#include <ola/Callback.h>
#include <ola/network/Socket.h>
#include <ola/OlaClientWrapper.h>

#include "tools/slp/Base.h"

#include <memory>
#include <string>
#include <vector>

namespace ola {
namespace slp {

using std::auto_ptr;
using std::string;


/**
 * An SLP Service
 */
struct SLPService {
  public:
    string name;
    uint16_t lifetime;

    SLPService(const string &name, uint16_t lifetime)
        : name(name),
          lifetime(lifetime) {
    }
};


class SLPClient {
  public:
    explicit SLPClient(ola::io::ConnectedDescriptor *descriptor);
    ~SLPClient() {}

    bool Setup();
    bool Stop();

    /**
     * Register a service
     */
    bool RegisterService(
        const string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const string&, uint16_t> *callback);

    /**
     * Register a service that persists beyond the lifetime of this client.
     */
    bool RegisterPersistentService(
        const string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const string&, uint16_t> *callback);

    /**
     * Find a service.
     */
    bool FindService(
        const string &service,
        SingleUseCallback2<void, const string&,
                           const vector<SLPService> &> *callback);

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
      m_socket.reset(TCPSocket::Connect("127.0.0.1", OLA_SLP_DEFAULT_PORT));
    }
};
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPCLIENT_H_
