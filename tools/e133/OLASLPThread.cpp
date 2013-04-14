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
 * OLASLPThread.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/e133/OLASLPThread.h>
#include <ola/slp/SLPClient.h>
#include <string>
#include <vector>
#include "slp/SLPPacketConstants.h"

namespace ola {
namespace e133 {

using std::string;
using std::vector;
using ola::TimeInterval;
using ola::slp::URLEntries;

/**
 * Create a new resolver thread. This doesn't actually start it.
 * @param ss the select server to use to handle the callbacks.
 */
OLASLPThread::OLASLPThread(ola::thread::ExecutorInterface *executor,
                           unsigned int discovery_interval)
    : BaseSLPThread(executor, discovery_interval),
      m_backoff_generator(new ola::ExponentialBackoffPolicy(
            TimeInterval(1, 0), TimeInterval(64, 0))),
      m_reconnect_timeout(ola::thread::INVALID_TIMEOUT) {
}


/**
 * Clean up
 */
OLASLPThread::~OLASLPThread() {
  Cleanup();
}


/**
 * Setup the SLP Thread
 */
bool OLASLPThread::Init() {
  if (!ConnectAndSetupClient())
    return false;
  return BaseSLPThread::Init();
}


/**
 * Clean up
 */
void OLASLPThread::Cleanup() {
  ShutdownClient();
  m_init_ok = false;
}

void OLASLPThread::RunDiscovery(InternalDiscoveryCallback *callback,
                                const string &service) {
  if (!m_slp_client.get()) {
    URLEntries urls;
    callback->Run(false, urls);
    return;
  }

  vector<string> scopes;
  scopes.push_back(RDNMET_SCOPE);
  m_slp_client->FindService(
      scopes, service,
      NewSingleCallback(this, &OLASLPThread::HandleDiscovery, callback));
}

void OLASLPThread::RegisterSLPService(RegistrationCallback *callback,
                                      const string& url,
                                      unsigned short lifetime) {
  if (!m_slp_client.get()) {
    callback->Run(false);
    return;
  }
  vector<string> scopes;
  scopes.push_back(RDNMET_SCOPE);
  m_slp_client->RegisterService(
      scopes, url, lifetime,
      NewSingleCallback(this, &OLASLPThread::HandleRegistration, callback));
}


void OLASLPThread::DeRegisterSLPService(RegistrationCallback *callback,
                                        const string& url) {
  if (!m_slp_client.get()) {
    callback->Run(false);
    return;
  }
  vector<string> scopes;
  scopes.push_back(RDNMET_SCOPE);
  m_slp_client->DeRegisterService(
      scopes, url,
      NewSingleCallback(this, &OLASLPThread::HandleDeRegistration, callback));
}

void OLASLPThread::SLPServerInfo(ServerInfoCallback *callback) {
  ola::slp::ServerInfo server_info;
  if (!m_slp_client.get()) {
    callback->Run(false, server_info);
    return;
  }
  bool ok = m_slp_client->GetServerInfo(
      NewSingleCallback(this, &OLASLPThread::HandleServerInfo, callback));
  if (!ok) {
    callback->Run(false, server_info);
    return;
  }
}

void OLASLPThread::ThreadStopping() {
  if (m_reconnect_timeout != ola::thread::INVALID_TIMEOUT) {
    m_ss.RemoveTimeout(m_reconnect_timeout);
    m_reconnect_timeout = ola::thread::INVALID_TIMEOUT;
  }
}

void OLASLPThread::HandleDiscovery(InternalDiscoveryCallback *callback,
                                   const string &status,
                                   const URLEntries &urls) {
  callback->Run(status.empty(), urls);
}

void OLASLPThread::HandleRegistration(RegistrationCallback *callback,
                                      const string &status,
                                      uint16_t error_code) {
  bool ok = status.empty() && error_code == ola::slp::SLP_OK;
  callback->Run(ok);
}

void OLASLPThread::HandleDeRegistration(RegistrationCallback *callback,
                                        const string &status,
                                        uint16_t error_code) {
  bool ok = status.empty() && error_code == ola::slp::SLP_OK;
  callback->Run(ok);
}

void OLASLPThread::HandleServerInfo(ServerInfoCallback *callback,
                                    const string &status,
                                    const ola::slp::ServerInfo &server_info) {
  callback->Run(status.empty(), server_info);
}

void OLASLPThread::SocketClosed() {
  OLA_WARN << "Lost connection to SLP server";
  ShutdownClient();
  m_reconnect_timeout = m_ss.RegisterSingleTimeout(
      m_backoff_generator.Next(),
      ola::NewSingleCallback(this, &OLASLPThread::AttemptSLPConnection));
}

void OLASLPThread::ShutdownClient() {
  if (m_slp_socket.get()) {
    m_ss.RemoveReadDescriptor(m_slp_socket.get());
  }

  if (m_slp_client.get()) {
    m_slp_client->Stop();
    m_slp_client.reset();
  }
  if (m_slp_socket.get()) {
    m_slp_socket.reset();
  }
}

bool OLASLPThread::ConnectAndSetupClient() {
  ola::network::IPV4SocketAddress target(
      ola::network::IPV4Address::Loopback(),
      ola::slp::OLA_SLP_DEFAULT_PORT);
  m_slp_socket.reset(ola::network::TCPSocket::Connect(target));
  if (!m_slp_socket.get()) {
    OLA_WARN << "Failed to connect to the OLA SLP Server at " << target;
    return false;
  }

  m_slp_client.reset(new ola::slp::SLPClient(m_slp_socket.get()));
  if (!m_slp_client->Setup()) {
    return false;
  }
  m_slp_socket->SetOnClose(
      ola::NewSingleCallback(this, &OLASLPThread::SocketClosed));
  m_ss.AddReadDescriptor(m_slp_socket.get());
  return true;
}


void OLASLPThread::AttemptSLPConnection() {
  OLA_INFO << "Attempting reconnection to SLP";
  // It's ok that this blocks, since the thread isn't able to make progress
  // until the connection is back anyway.
  if (ConnectAndSetupClient()) {
    ReRegisterAllServices();
  } else {
    m_reconnect_timeout = m_ss.RegisterSingleTimeout(
        m_backoff_generator.Next(),
        ola::NewSingleCallback(this, &OLASLPThread::AttemptSLPConnection));
  }
}
}  // e133
}  // ola
