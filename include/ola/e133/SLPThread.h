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
 * SLPThread.h
 * Copyright (C) 2011 Simon Newton
 *
 * The BaseSLPThread abstracts away all the SLP code in an implementation
 * independant manner. There are two implementations, one that uses openslp and
 * the other that uses OLA's SLP server.
 *
 * Like the name implies, the SLPThread starts up a new thread to handle SLP
 * operations. You simple have to call RegisterDevice / RegisterController
 * once, and the thread will take care of re-registering your service before
 * the lifetime expires.
 *
 * To de-register the service entirely call DeRegisterDevice /
 * DeRegisterController.
 *
 * The Register* and DeRegister* methods can be called from any thread.
 * The callbacks will run the in Executor passed to the constructor.
 */

#ifndef INCLUDE_OLA_E133_SLPTHREAD_H_
#define INCLUDE_OLA_E133_SLPTHREAD_H_

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/io/SelectServer.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Socket.h>
#include <ola/rdm/UID.h>
#include <ola/slp/SLPClient.h>
#include <ola/slp/URLEntry.h>
#include <ola/thread/ExecutorInterface.h>
#include <ola/thread/Thread.h>

#include <map>
#include <string>
#include <vector>

namespace ola {
namespace e133 {

using std::string;
using ola::rdm::UID;
using ola::network::IPV4Address;
using ola::slp::URLEntries;

class SLPThreadServerInfo : public ola::slp::ServerInfo {
 public:
    string backend_type;

    SLPThreadServerInfo() : ServerInfo() {}

    SLPThreadServerInfo(const SLPThreadServerInfo &server_info)
        : ServerInfo(server_info),
          backend_type(server_info.backend_type) {
    }

    explicit SLPThreadServerInfo(const ola::slp::ServerInfo &server_info)
        : ServerInfo(server_info) {
    }
};

/**
 * The base class for a thread which handles all the SLP stuff.
 */
class BaseSLPThread: public ola::thread::Thread {
 public:
    typedef ola::BaseCallback1<void, bool> RegistrationCallback;
    typedef ola::Callback2<void, bool, const ola::slp::URLEntries&>
        DiscoveryCallback;
    typedef ola::SingleUseCallback2<void, bool, const SLPThreadServerInfo&>
        ServerInfoCallback;

    // Ownership is not transferred.
    explicit BaseSLPThread(
        ola::thread::ExecutorInterface *executor,
        unsigned int discovery_interval = DEFAULT_DISCOVERY_INTERVAL_SECONDS);
    virtual ~BaseSLPThread();

    // These must be called before Init(); Once Initialized the callbacks can't
    // be changed. Ownership of the callback is transferred.
    bool SetNewControllerCallback(DiscoveryCallback *callback);
    bool SetNewDeviceCallback(DiscoveryCallback *callback);

    void RegisterDevice(RegistrationCallback *callback,
                        const IPV4Address &address,
                        const UID &uid,
                        uint16_t lifetime);

    void RegisterController(RegistrationCallback *callback,
                            const IPV4Address &address,
                            uint16_t lifetime);

    void DeRegisterDevice(RegistrationCallback *callback,
                          const IPV4Address &address,
                          const UID &uid);

    void DeRegisterController(RegistrationCallback *callback,
                              const IPV4Address &address);

    void ServerInfo(ServerInfoCallback *callback);

    void RunDeviceDiscoveryNow();

    virtual bool Init();
    bool Start();
    bool Join(void *ptr = NULL);
    virtual void Cleanup() = 0;

    static const unsigned int DEFAULT_DISCOVERY_INTERVAL_SECONDS;

 protected:
    typedef ola::SingleUseCallback2<void, bool, const ola::slp::URLEntries&>
        InternalDiscoveryCallback;

    ola::io::SelectServer m_ss;
    ola::thread::ExecutorInterface *m_executor;

    void *Run();
    void RunCallbackInExecutor(RegistrationCallback *callback, bool ok);

    // The sub class provides these, they are allowed to block.
    virtual void RunDiscovery(InternalDiscoveryCallback *callback,
                              const string &service) = 0;
    virtual void RegisterSLPService(RegistrationCallback *callback,
                                    const string& url,
                                    uint16_t lifetime) = 0;
    virtual void DeRegisterSLPService(RegistrationCallback *callback,
                                      const string& url) = 0;
    virtual void SLPServerInfo(ServerInfoCallback *callback) = 0;

    // Called after the SelectServer has finished and just before the thread
    // completes.
    virtual void ThreadStopping() {}

    // This enables us to limit the refresh-time, 0 means the implementation
    // doesn't have a min-refresh-time.
    virtual uint16_t MinRefreshTime() { return 0; }

    void ReRegisterAllServices();

    static const char RDNMET_SCOPE[];

 private:
    typedef struct {
      uint16_t lifetime;
      ola::thread::timeout_id timeout;
    } URLRegistrationState;
    typedef std::map<string, URLRegistrationState> URLStateMap;

    typedef struct {
      DiscoveryCallback *callback;
      ola::thread::timeout_id timeout;
    } DiscoveryState;
    typedef std::map<string, DiscoveryState> DiscoveryStateMap;

    URLStateMap m_url_map;
    DiscoveryStateMap m_discovery_callbacks;
    bool m_init_ok;
    unsigned int m_discovery_interval;

    // discovery methods
    void AddDiscoveryCallback(string service, DiscoveryCallback *callback);
    void StartDiscoveryProcess();
    void RemoveDiscoveryTimeout(DiscoveryState *state);
    void RunDiscoveryForService(const string service);
    void ForceDiscovery(const string service);
    void DiscoveryComplete(const string service, bool result,
                           const URLEntries &urls);
    void RunDiscoveryCallback(DiscoveryCallback *callback, bool result,
                              const URLEntries *urls);
    void DiscoveryTriggered(const string service);

    // register / deregister methods
    void RegisterService(RegistrationCallback *callback, const string url,
                         uint16_t lifetime);
    void RegistrationComplete(RegistrationCallback *callback, string url,
                              bool ok);
    void DeRegisterService(RegistrationCallback *callback, const string url);
    void ReRegisterService(string url);
    void CompleteCallback(RegistrationCallback *callback, bool ok);

    // server info methods
    void GetServerInfo(ServerInfoCallback *callback);
    void HandleServerInfo(ServerInfoCallback *callback, bool ok,
                          const SLPThreadServerInfo &service_info);
    void CompleteServerInfo(ServerInfoCallback *callback,
                            bool ok,
                            const SLPThreadServerInfo *server_info_ptr);

    // helper methods
    static string GetDeviceURL(const IPV4Address& address, const UID &uid);
    static string GetControllerURL(const IPV4Address& address);
    static uint16_t ClampLifetime(const string &url, uint16_t lifetime);

    static const uint16_t MIN_SLP_LIFETIME;
    static const uint16_t SA_REREGISTRATION_TIME;
    static const char E133_DEVICE_SLP_SERVICE_NAME[];
    static const char E133_CONTROLLER_SLP_SERVICE_NAME[];

    DISALLOW_COPY_AND_ASSIGN(BaseSLPThread);
};


/**
 * Creates new SLPThreads based on a command line flag.
 */
class SLPThreadFactory {
 public:
    static BaseSLPThread* NewSLPThread(
      ola::thread::ExecutorInterface *ss,
      unsigned int discovery_interval =
          BaseSLPThread::DEFAULT_DISCOVERY_INTERVAL_SECONDS);

 private:
    DISALLOW_COPY_AND_ASSIGN(SLPThreadFactory);
};
}  // namespace e133
}  // namespace ola
#endif  // INCLUDE_OLA_E133_SLPTHREAD_H_
