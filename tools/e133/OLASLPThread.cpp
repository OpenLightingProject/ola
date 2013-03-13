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
#include <string>
#include <vector>
#include "tools/e133/OLASLPThread.h"
#include "tools/slp/SLPClient.h"
#include "tools/slp/SLPPacketConstants.h"


using std::string;
using std::vector;
using ola::slp::URLEntries;

/**
 * Create a new resolver thread. This doesn't actually start it.
 * @param ss the select server to use to handle the callbacks.
 */
OLASLPThread::OLASLPThread(ola::thread::ExecutorInterface *executor,
                           unsigned int discovery_interval)
    : BaseSLPThread(executor, discovery_interval),
      m_init_ok(false) {
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
  return BaseSLPThread::Init();
}


/**
 * Clean up
 */
void OLASLPThread::Cleanup() {
  if (m_slp_socket.get()) {
    m_ss.AddReadDescriptor(m_slp_socket.get());
  }

  if (m_slp_client.get()) {
    m_slp_client->Stop();
    m_slp_client.reset();
  }
  if (m_slp_socket.get()) {
    m_slp_socket.reset();
  }
  m_init_ok = false;
}

void OLASLPThread::RunDiscovery(InternalDiscoveryCallback *callback,
                                const string &service) {
  vector<string> scopes;
  scopes.push_back(RDNMET_SCOPE);
  m_slp_client->FindService(
      scopes, service,
      NewSingleCallback(this, &OLASLPThread::HandleDiscovery, callback));
}

void OLASLPThread::RegisterSLPService(RegistrationCallback *callback,
                                      const string& url,
                                      unsigned short lifetime) {
  vector<string> scopes;
  scopes.push_back(RDNMET_SCOPE);
  m_slp_client->RegisterService(
      scopes, url, lifetime,
      NewSingleCallback(this, &OLASLPThread::HandleRegistration, callback));
}


void OLASLPThread::DeRegisterSLPService(RegistrationCallback *callback,
                                        const string& url) {
  vector<string> scopes;
  scopes.push_back(RDNMET_SCOPE);
  m_slp_client->DeRegisterService(
      scopes, url,
      NewSingleCallback(this, &OLASLPThread::HandleDeRegistration, callback));
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

void OLASLPThread::SocketClosed() {
  m_ss.Terminate();
}
