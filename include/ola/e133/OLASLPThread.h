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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * OLASLPThread.h
 * Copyright (C) 2011 Simon Newton
 *
 * An implementation of BaseSLPThread that uses OLA's SLP server.
 */

#ifndef INCLUDE_OLA_E133_OLASLPTHREAD_H_
#define INCLUDE_OLA_E133_OLASLPTHREAD_H_

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/e133/SLPThread.h>
#include <ola/network/Socket.h>
#include <ola/network/TCPSocket.h>
#include <ola/slp/SLPClient.h>
#include <ola/thread/ExecutorInterface.h>
#include <ola/thread/Thread.h>
#include <ola/util/Backoff.h>

#include <memory>
#include <string>

namespace ola {
namespace e133 {

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
                            uint16_t lifetime);
    void DeRegisterSLPService(RegistrationCallback *callback,
                              const string& url);
    void SLPServerInfo(ServerInfoCallback *callback);

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
    void HandleServerInfo(ServerInfoCallback *callback,
                          const string &status,
                          const ola::slp::ServerInfo &server_info);

    void SocketClosed();
    void ShutdownClient();
    bool ConnectAndSetupClient();
    void AttemptSLPConnection();

    DISALLOW_COPY_AND_ASSIGN(OLASLPThread);
};
}  // namespace e133
}  // namespace ola
#endif  // INCLUDE_OLA_E133_OLASLPTHREAD_H_
