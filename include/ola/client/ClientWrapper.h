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
 * ClientWrapper.h
 * This provides a helpful wrapper for the OlaClient & OlaCallbackClient
 * classes.
 * Copyright (C) 2005 Simon Newton
 *
 * The OlaClientWrapper classes takes care of setting up the socket, select
 * server and client for you.
 */

/**
 * @file ClientWrapper.h
 * @brief Helper classes for managing OLA clients.
 */

#ifndef INCLUDE_OLA_CLIENT_CLIENTWRAPPER_H_
#define INCLUDE_OLA_CLIENT_CLIENTWRAPPER_H_

// On MinGW, SocketAddress.h pulls in WinSock2.h, which needs to be after
// WinSock2.h, hence this order
#include <ola/network/SocketAddress.h>
#include <ola/network/TCPSocket.h>

#include <ola/AutoStart.h>
#include <ola/Callback.h>
#include <ola/client/OlaClient.h>
#include <ola/io/SelectServer.h>

#include <memory>

namespace ola {
namespace client {

/**
 * @brief The base Client Wrapper class.
 *
 * This encapsulates the calls required to setup a connection to olad.
 */
class BaseClientWrapper {
 public:
  typedef Callback0<void> CloseCallback;

  BaseClientWrapper();
  virtual ~BaseClientWrapper();

  /**
   * @brief Set the callback to be run when the client socket is closed.
   *
   * The default action is to terminate the SelectServer. By setting a callback
   * you can override this behavior.
   *
   * @param callback the Callback to run, ownership is transferred.
   */
  void SetCloseCallback(CloseCallback *callback);

  /**
   * @brief Get the SelectServer used by this client.
   * @returns A pointer to a SelectServer, ownership isn't transferred.
   */
  ola::io::SelectServer *GetSelectServer() { return &m_ss; }

  /**
   * @brief Setup the client.
   * @returns true on success, false on failure.
   */
  bool Setup();

  /**
   * @brief Reset the connection to the server.
   * @returns true if the reset succeeded, false otherwise.
   */
  bool Cleanup();

  /**
   * @brief Called internally when the client socket is closed.
   * @internal
   */
  void SocketClosed();

 protected:
  std::auto_ptr<ola::network::TCPSocket> m_socket;

 private:
  ola::io::SelectServer m_ss;
  std::auto_ptr<CloseCallback> m_close_callback;

  virtual void CreateClient() = 0;
  virtual bool StartupClient() = 0;
  virtual void InitSocket() = 0;
};


/**
 * @brief A templatized ClientWrapper.
 */
template <typename ClientClass>
class GenericClientWrapper: public BaseClientWrapper {
 public:
  explicit GenericClientWrapper(bool auto_start = true):
      BaseClientWrapper(),
      m_auto_start(auto_start) {
  }
  ~GenericClientWrapper() {}

  /**
   * @brief Return the underlying client object
   */
  ClientClass *GetClient() const { return m_client.get(); }

 private:
  std::auto_ptr<ClientClass> m_client;
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
      m_socket.reset(ola::network::TCPSocket::Connect(
          ola::network::IPV4SocketAddress(
            ola::network::IPV4Address::Loopback(),
           OLA_DEFAULT_PORT)));
    }
    if (m_socket.get()) {
      m_socket->SetNoDelay();
    }
  }
};

/**
 * @brief A ClientWrapper that uses the OlaClient.
 */
typedef GenericClientWrapper<OlaClient> OlaClientWrapper;

}  // namespace client
}  // namespace ola
#endif  // INCLUDE_OLA_CLIENT_CLIENTWRAPPER_H_
