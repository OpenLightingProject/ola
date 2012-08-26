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

#include <ola/OlaClient.h>
#include <ola/OlaCallbackClient.h>
#include <ola/io/SelectServer.h>
#include <ola/network/Socket.h>

namespace ola {

using ola::io::SelectServer;
using ola::network::TCPSocket;

/*
 * The base class, not used directly.
 */
class BaseClientWrapper {
  public:
    explicit BaseClientWrapper(bool auto_start);
    virtual ~BaseClientWrapper();

    SelectServer *GetSelectServer() const { return m_ss; }

    bool Setup();
    bool Cleanup();

    void SocketClosed();

  protected:
    TCPSocket *m_socket;

  private:
    virtual void CreateClient() = 0;
    virtual bool StartupClient() = 0;

    bool m_auto_start;
    SelectServer *m_ss;
};


/*
 * This wrapper uses the OlaClient class.
 */
template <typename client_class>
class GenericClientWrapper: public BaseClientWrapper {
  public:
    explicit GenericClientWrapper(bool auto_start = true):
        BaseClientWrapper(auto_start),
        m_client(NULL) {
    }
    ~GenericClientWrapper() {
      if (m_client)
        delete m_client;
    }

    client_class *GetClient() const { return m_client; }

  private:
    client_class *m_client;

    void CreateClient() {
      if (!m_client) {
        m_client = new client_class(m_socket);
      }
    }
    bool StartupClient() {
      return m_client->Setup();
    }
};

typedef GenericClientWrapper<OlaClient> OlaClientWrapper;
// for historical reasons we typedef SimpleClient to OlaClientWrapper
typedef GenericClientWrapper<OlaClient> SimpleClient;


typedef GenericClientWrapper<OlaCallbackClient> OlaCallbackClientWrapper;
}  // ola
#endif  // OLA_OLACLIENTWRAPPER_H_
