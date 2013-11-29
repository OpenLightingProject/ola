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
 * OlaClientWrapper.h
 * This provides a helpful wrapper for the OlaClient & OlaCallbackClient
 * classes.
 * Copyright (C) 2005-2008 Simon Newton
 *
 * The OlaClientWrapper classes takes care of setting up the socket, select
 * server and client for you.
 */

#ifndef OLA_OLACLIENTWRAPPER_H_
#define OLA_OLACLIENTWRAPPER_H_

#include <ola/AutoStart.h>
#include <ola/OlaCallbackClient.h>
#include <ola/client/OlaClient.h>
#include <ola/io/SelectServer.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/TCPSocket.h>

#include <memory>

namespace ola {
namespace client {

using ola::io::SelectServer;
using ola::network::TCPSocket;
using std::auto_ptr;

/*
 * The base class, not used directly.
 */
class BaseClientWrapper {
 public:
    BaseClientWrapper() {}
    virtual ~BaseClientWrapper();

    SelectServer *GetSelectServer() { return &m_ss; }

    bool Setup();
    bool Cleanup();
    void SocketClosed();

 protected:
    auto_ptr<TCPSocket> m_socket;

 private:
    SelectServer m_ss;

    virtual void CreateClient() = 0;
    virtual bool StartupClient() = 0;
    virtual void InitSocket() = 0;
};


/*
 * This wrapper uses the OlaClient class.
 */
template <typename ClientClass>
class GenericClientWrapper: public BaseClientWrapper {
 public:
    explicit GenericClientWrapper(bool auto_start = true):
        BaseClientWrapper(),
        m_auto_start(auto_start) {
    }
    ~GenericClientWrapper() {}

    ClientClass *GetClient() const { return m_client.get(); }

 private:
    auto_ptr<ClientClass> m_client;
    bool m_auto_start;

    void CreateClient() {
      if (!m_client.get()) {
        m_client.reset(new ClientClass(m_socket.get()));
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
      if (m_auto_start) {
        m_socket.reset(ola::client::ConnectToServer(OLA_DEFAULT_PORT));
      } else {
        m_socket.reset(TCPSocket::Connect(
            ola::network::IPV4SocketAddress(
              ola::network::IPV4Address::Loopback(),
             OLA_DEFAULT_PORT)));
      }
      m_socket->SetNoDelay();
    }
};

typedef GenericClientWrapper<OlaClient> OlaClientWrapper;
}  // namespace client

// Legacy
typedef ola::client::GenericClientWrapper<OlaCallbackClient>
    OlaCallbackClientWrapper;

}  // namespace ola
#endif  // OLA_OLACLIENTWRAPPER_H_
