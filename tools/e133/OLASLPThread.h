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
 * OLASLPThread.h
 * Copyright (C) 2011 Simon Newton
 *
 * An implementation of BaseSLPThread that uses OLA's SLP server.
 */

#include <ola/Callback.h>
#include <ola/network/Socket.h>
#include <ola/network/TCPSocket.h>
#include <ola/thread/ExecutorInterface.h>
#include <ola/thread/Thread.h>
#include <ola/util/Backoff.h>
#include <ola/slp/SLPClient.h>

#include <memory>
#include <string>

#include "tools/e133/SLPThread.h"

#ifndef TOOLS_E133_OLASLPTHREAD_H_
#define TOOLS_E133_OLASLPTHREAD_H_

using std::string;
using std::auto_ptr;

class OLASLPThread: public BaseSLPThread {
  public:
    // Ownership of the discovery_callback is transferred.
    OLASLPThread(
        ola::thread::ExecutorInterface *ss,
        unsigned int discovery_interval = DEFAULT_DISCOVERY_INTERVAL_SECONDS);
    ~OLASLPThread();

    bool Init();
    void Cleanup();

  protected:
    void RunDiscovery(InternalDiscoveryCallback *callback,
                      const string &service);
    void RegisterSLPService(RegistrationCallback *callback,
                            const string& url,
                            unsigned short lifetime);
    void DeRegisterSLPService(RegistrationCallback *callback,
                              const string& url);
    void ThreadStopping();

  private:
    bool m_init_ok;
    ola::BackoffGenerator m_backoff_generator;
    auto_ptr<ola::network::TCPSocket> m_slp_socket;
    auto_ptr<ola::slp::SLPClient> m_slp_client;
    ola::thread::timeout_id m_reconnect_timeout;

    void HandleDiscovery(InternalDiscoveryCallback *callback,
                         const string &status,
                         const URLEntries &urls);
    void HandleRegistration(RegistrationCallback *callback,
                            const string &status, uint16_t error_code);
    void HandleDeRegistration(RegistrationCallback *callback,
                              const string &status, uint16_t error_code);
    void SocketClosed();
    void ShutdownClient();
    bool ConnectAndSetupClient();
    void AttemptSLPConnection();
};

#endif  // TOOLS_E133_OLASLPTHREAD_H_
