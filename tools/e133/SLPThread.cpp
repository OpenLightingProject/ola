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
 * SLPThread.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/e133/SLPThread.h>
#include <ola/slp/URLEntry.h>
#include <ola/stl/STLUtils.h>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include "tools/e133/SLPConstants.h"

namespace ola {
namespace e133 {

// Constants from the E1.33 doc.
const uint16_t BaseSLPThread::MIN_SLP_LIFETIME = 300;
const char BaseSLPThread::RDNMET_SCOPE[] = "RDMNET";
const char BaseSLPThread::E133_DEVICE_SLP_SERVICE_NAME[] =
    "service:rdmnet-device";
const char BaseSLPThread::E133_CONTROLLER_SLP_SERVICE_NAME[] =
    "service:rdmnet-ctrl";
const uint16_t BaseSLPThread::SA_REREGISTRATION_TIME = 30;

// Our own constants.
const unsigned int BaseSLPThread::DEFAULT_DISCOVERY_INTERVAL_SECONDS = 60;

using ola::NewSingleCallback;
using ola::STLFind;
using ola::STLReplace;
using ola::slp::URLEntries;
using std::auto_ptr;
using std::pair;


/**
 * If not executor is provided, the callbacks are run in the slp thread.
 */
BaseSLPThread::BaseSLPThread(ola::thread::ExecutorInterface *executor,
                             unsigned int discovery_interval)
    : Thread(),
      m_executor(executor),
      m_init_ok(false),
      m_discovery_interval(discovery_interval) {
}


BaseSLPThread::~BaseSLPThread() {
  DiscoveryStateMap::iterator iter = m_discovery_callbacks.begin();
  for (; iter != m_discovery_callbacks.end(); ++iter) {
    delete iter->second.callback;
  }
}

/**
 * Set the handler to be called when new Controllers are discovered
 * Ownership of the callback is transferred.
 */
bool BaseSLPThread::SetNewControllerCallback(DiscoveryCallback *callback) {
  if (m_init_ok) {
    OLA_WARN << "Attempt to set the Controller callback once Init() has run";
    return false;
  }
  AddDiscoveryCallback(E133_CONTROLLER_SLP_SERVICE_NAME, callback);
  return true;
}

/**
 * Set the handler to be called when new Devices are discovered.
 * Ownership of the callback is transferred.
 */
bool BaseSLPThread::SetNewDeviceCallback(DiscoveryCallback *callback) {
  if (m_init_ok) {
    OLA_WARN << "Attempt to set the Device callback once Init() has run";
    return false;
  }
  AddDiscoveryCallback(E133_DEVICE_SLP_SERVICE_NAME, callback);
  return true;
}

/**
 * Initialized this SLP thread.
 */
bool BaseSLPThread::Init() {
  m_init_ok = true;
  return true;
}

/**
 * Start the SLP thread.
 */
bool BaseSLPThread::Start() {
  if (!m_init_ok) {
    OLA_WARN << "Called to SLPThread::Start() without a call to Init()";
    return false;
  }
  m_ss.Execute(NewSingleCallback(this, &BaseSLPThread::StartDiscoveryProcess));
  return ola::thread::Thread::Start();
}

/**
 * Stop the SLP thread.
 */
bool BaseSLPThread::Join(void *ptr) {
  m_ss.Terminate();
  return ola::thread::Thread::Join(ptr);
}

/**
 * The entry point for the new thread.
 */
void *BaseSLPThread::Run() {
  m_ss.Run();
  ThreadStopping();
  return NULL;
}

/*
 * Register an E1.33 device in SLP.
 * @param callback the callback to run when the registration is complete
 * @param address the IPV4Address of the device.
 * @param uid the E1.33 UID of the device.
 * @param lifetime the lifetime to use when registering with the server.
 */
void BaseSLPThread::RegisterDevice(RegistrationCallback *callback,
                                   const IPV4Address &address,
                                   const UID &uid,
                                   uint16_t lifetime) {
  const string url = GetDeviceURL(address, uid);
  m_ss.Execute(NewSingleCallback(
        this, &BaseSLPThread::RegisterService, callback, url, lifetime));
}

/*
 * Register an E1.33 controller in SLP.
 * @param callback the callback to run when the registration is complete
 * @param address the IPV4Address of the device.
 * @param uid the E1.33 UID of the device.
 * @param lifetime the lifetime to use when registering with the server.
 */
void BaseSLPThread::RegisterController(RegistrationCallback *callback,
                                       const IPV4Address &address,
                                       uint16_t lifetime) {
  const string url = GetControllerURL(address);
  m_ss.Execute(NewSingleCallback(
        this, &BaseSLPThread::RegisterService, callback, url, lifetime));
}

/*
 * DeRegister an E1.33 device.
 */
void BaseSLPThread::DeRegisterDevice(RegistrationCallback *callback,
                                     const IPV4Address &address,
                                     const UID &uid) {
  m_ss.Execute(NewSingleCallback(
      this,
      &BaseSLPThread::DeRegisterService,
      callback,
      GetDeviceURL(address, uid)));
}

/*
 * DeRegister an E1.33 controller.
 */
void BaseSLPThread::DeRegisterController(RegistrationCallback *callback,
                                         const IPV4Address &address) {
  m_ss.Execute(NewSingleCallback(
      this,
      &BaseSLPThread::DeRegisterService,
      callback,
      GetControllerURL(address)));
}


void BaseSLPThread::ServerInfo(ServerInfoCallback *callback) {
  m_ss.Execute(NewSingleCallback(
      this,
      &BaseSLPThread::GetServerInfo,
      callback));
}


/**
 * Trigger E1.33 device discovery immediately.
 */
void BaseSLPThread::RunDeviceDiscoveryNow() {
  m_ss.Execute(NewSingleCallback(this, &BaseSLPThread::ForceDiscovery,
                                 string(E133_DEVICE_SLP_SERVICE_NAME)));
}


/**
 * Schedule the callback to run in the Executor thread.
 */
void BaseSLPThread::RunCallbackInExecutor(RegistrationCallback *callback,
                                          bool ok) {
  if (!callback) {
    return;
  }

  if (m_executor) {
    m_executor->Execute(
        NewSingleCallback(this, &BaseSLPThread::CompleteCallback, callback,
                          ok));
  } else {
    callback->Run(ok);
  }
}


/**
 * Re-register all services.
 */
void BaseSLPThread::ReRegisterAllServices() {
  URLStateMap::const_iterator iter = m_url_map.begin();
  for (; iter != m_url_map.end(); ++iter) {
    OLA_INFO << "Calling re-registering " << iter->first;
    RegisterSLPService(
        NewSingleCallback(this, &BaseSLPThread::RegistrationComplete,
                          reinterpret_cast<RegistrationCallback*>(NULL),
                          iter->first),
        iter->first, iter->second.lifetime);
  }
}


/**
 * Attach or remove a discovery handler for a particular service. This can only
 * run prior to Init().
 */
void BaseSLPThread::AddDiscoveryCallback(string service,
                                         DiscoveryCallback *callback) {
  if (callback) {
    DiscoveryState new_state = {callback, ola::thread::INVALID_TIMEOUT};
    pair<DiscoveryStateMap::iterator, bool> p = m_discovery_callbacks.insert(
        DiscoveryStateMap::value_type(service, new_state));
    if (!p.second) {
      // Not inserted, clean up the old one.
      RemoveDiscoveryTimeout(&(p.first->second));
      if (p.first->second.callback) {
        delete p.first->second.callback;
      }
      p.first->second.callback = callback;
    }
  } else {
    // Remove the callback for this service.
    DiscoveryStateMap::iterator iter = m_discovery_callbacks.find(service);
    if (iter != m_discovery_callbacks.end()) {
      RemoveDiscoveryTimeout(&(iter->second));
      if (iter->second.callback)
        delete iter->second.callback;
    }
    m_discovery_callbacks.erase(iter);
  }
}

void BaseSLPThread::StartDiscoveryProcess() {
  OLA_INFO << "Starting discovery process";
  DiscoveryStateMap::iterator iter = m_discovery_callbacks.begin();
  for (; iter != m_discovery_callbacks.end(); ++iter) {
    RunDiscoveryForService(iter->first);
  }
}


/*
 * Remove a timeout associated with a discovery service.
 */
void BaseSLPThread::RemoveDiscoveryTimeout(DiscoveryState *state) {
  if (state->timeout != ola::thread::INVALID_TIMEOUT) {
    m_ss.RemoveTimeout(state->timeout);
    state->timeout = ola::thread::INVALID_TIMEOUT;
  }
}

/**
 * Run discovery for a particular service. This calls into the child class.
 */
void BaseSLPThread::RunDiscoveryForService(const string service) {
  OLA_INFO << "running discovery for " << service;
  RunDiscovery(
      NewSingleCallback(this, &BaseSLPThread::DiscoveryComplete, service),
      service);
}


/**
 * Run discovery for the service immediately.
 */
void BaseSLPThread::ForceDiscovery(const string service) {
  DiscoveryState *state = STLFind(&m_discovery_callbacks, service);
  if (!state)
    return;
  RemoveDiscoveryTimeout(state);
  RunDiscoveryForService(service);
}

void BaseSLPThread::DiscoveryComplete(const string service,
                                      bool result,
                                      const URLEntries &urls) {
  DiscoveryState *state = STLFind(&m_discovery_callbacks, service);
  if (!state)
    return;

  // schedule retry
  RemoveDiscoveryTimeout(state);
  state->timeout = m_ss.RegisterSingleTimeout(
      m_discovery_interval * 1000,
      ola::NewSingleCallback(this, &BaseSLPThread::DiscoveryTriggered,
                             service));

  if (!state->callback) {
    return;
  }

  if (m_executor) {
    // run in exec
    const URLEntries *urls_ptr = new URLEntries(urls);
    m_executor->Execute(
        NewSingleCallback(this, &BaseSLPThread::RunDiscoveryCallback,
                          state->callback, result, urls_ptr));
  } else {
    state->callback->Run(result, urls);
  }
}

/**
 * Run the client's discovery callback.
 */
void BaseSLPThread::RunDiscoveryCallback(DiscoveryCallback *callback,
                                         bool result,
                                         const URLEntries *urls_ptr) {
  auto_ptr<const URLEntries> urls(urls_ptr);
  callback->Run(result, *urls_ptr);
}


/**
 * Called when the discovery timer triggers. This usually means that the
 * lifetime of one of the discovered urls has expired and we need to check if
 * it's still active.
 */
void BaseSLPThread::DiscoveryTriggered(const string service) {
  OLA_INFO << "scheduled next discovery run";
  DiscoveryState *state = STLFind(&m_discovery_callbacks, service);
  if (!state)
    return;
  state->timeout = ola::thread::INVALID_TIMEOUT;
  RunDiscoveryForService(service);
}

/**
 * Register the URL with SLP. This is called within our thread.
 */
void BaseSLPThread::RegisterService(RegistrationCallback *callback,
                                    const string url,
                                    unsigned short lifetime) {
  lifetime = ClampLifetime(url, lifetime);
  uint16_t min_lifetime = MinRefreshTime();
  if (min_lifetime != 0 && lifetime < min_lifetime) {
    OLA_INFO << "Min interval from DA is " << min_lifetime;
    lifetime = min_lifetime;
  }

  URLStateMap::iterator iter = m_url_map.find(url);
  if (iter != m_url_map.end()) {
    if (iter->second.lifetime == lifetime) {
      OLA_INFO << "New lifetime of " << url
               << " matches current registration, ignoring update";
      RunCallbackInExecutor(callback, true);
      return;
    }
    iter->second.lifetime = lifetime;
    if (iter->second.timeout != ola::thread::INVALID_TIMEOUT)
      m_ss.RemoveTimeout(iter->second.timeout);
  } else {
    URLRegistrationState url_state = {lifetime, ola::thread::INVALID_TIMEOUT};
    STLReplace(&m_url_map, url, url_state);
  }

  OLA_INFO << "Calling register for " << url;
  RegisterSLPService(
      NewSingleCallback(this, &BaseSLPThread::RegistrationComplete, callback,
                        url),
      url, lifetime);
}


/**
 * Called when a register request completes.
 */
void BaseSLPThread::RegistrationComplete(RegistrationCallback *callback,
                                         string url, bool ok) {
  URLRegistrationState *url_state = STLFind(&m_url_map, url);
  if (url_state) {
    uint16_t lifetime = url_state->lifetime - SA_REREGISTRATION_TIME;
    url_state->timeout = m_ss.RegisterSingleTimeout(
        (lifetime - 1) * 1000,
        ola::NewSingleCallback(this, &BaseSLPThread::ReRegisterService, url));
  } else {
    OLA_WARN << "Unable to find matching URLRegistrationState for " << url;
  }
  RunCallbackInExecutor(callback, ok);
}

/**
 * De-Register the URL with SLP. This is called within our thread.
 */
void BaseSLPThread::DeRegisterService(RegistrationCallback *callback,
                                      const string url) {
  URLStateMap::iterator iter = m_url_map.find(url);
  if (iter != m_url_map.end()) {
    OLA_INFO << "Removing " << url << " from state map";
    if (iter->second.timeout != ola::thread::INVALID_TIMEOUT)
      m_ss.RemoveTimeout(iter->second.timeout);
    m_url_map.erase(iter);
  }

  DeRegisterSLPService(
      ola::NewSingleCallback(this, &BaseSLPThread::RunCallbackInExecutor,
                             callback),
      url);
}

/**
 * Called when the lifetime for a service has expired and we need to register
 * it again.
 */
void BaseSLPThread::ReRegisterService(string url) {
  OLA_INFO << "Registering " << url << " again";
  URLRegistrationState *url_state = STLFind(&m_url_map, url);
  if (url_state) {
    RegisterSLPService(
        NewSingleCallback(this, &BaseSLPThread::RegistrationComplete,
                          reinterpret_cast<RegistrationCallback*>(NULL),
                          url),
        url, url_state->lifetime);
  }
}

/**
 * This should only ever be in the Executor thread.
 */
void BaseSLPThread::CompleteCallback(RegistrationCallback *callback,
                                     bool ok) {
  callback->Run(ok);
}


/**
 * Get the SLP Server info. This runs in our thread.
 */
void BaseSLPThread::GetServerInfo(ServerInfoCallback *callback) {
  SLPServerInfo(
      NewSingleCallback(this, &BaseSLPThread::HandleServerInfo, callback));
}


/**
 * Handle the ServerInfo response.
 */
void BaseSLPThread::HandleServerInfo(ServerInfoCallback *callback, bool ok,
                                     const SLPThreadServerInfo &server_info) {
  if (m_executor) {
    const SLPThreadServerInfo *server_info_ptr = new SLPThreadServerInfo(
        server_info);
    m_executor->Execute(
        NewSingleCallback(this, &BaseSLPThread::CompleteServerInfo,
                          callback, ok, server_info_ptr));
  } else {
    callback->Run(ok, server_info);
  }
};


/**
 * Runs on the executor thread.
 */
void BaseSLPThread::CompleteServerInfo(
    ServerInfoCallback *callback,
    bool ok,
    const SLPThreadServerInfo *server_info_ptr) {
  auto_ptr<const ola::slp::ServerInfo> service_info(server_info_ptr);
  callback->Run(ok, *server_info_ptr);
}


/**
 * Generate an E1.33 Device URL.
 */
string BaseSLPThread::GetDeviceURL(const IPV4Address& address, const UID &uid) {
  std::ostringstream str;
  str << E133_DEVICE_SLP_SERVICE_NAME << "://" << address << "/"
      << std::setfill('0') << std::setw(4) << std::hex
      << uid.ManufacturerId() << std::setw(8) << uid.DeviceId();
  return str.str();
}

/**
 * Generate an E1.33 Controller URL.
 */
string BaseSLPThread::GetControllerURL(const IPV4Address& address) {
  std::ostringstream str;
  str << E133_CONTROLLER_SLP_SERVICE_NAME << "://" << address;
  return str.str();
}

/**
 * Clamp the SLP lifetime to a sane value.
 */
uint16_t BaseSLPThread::ClampLifetime(const string &url, uint16_t lifetime) {
  if (lifetime < MIN_SLP_LIFETIME) {
    OLA_WARN << "Lifetime of " << url
             << " is less than the min E1.33 SLP lifetime, forcing to "
             << MIN_SLP_LIFETIME;
    lifetime = MIN_SLP_LIFETIME;
  }
  return lifetime;
}
}  // e133
}  // ola
