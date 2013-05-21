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
 * SLPClient.cpp
 * Implementation of SLPClient
 * Copyright (C) 2012 Simon Newton
 */

#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/slp/SLPClient.h"
#include "slp/SLPClientCore.h"

namespace ola {
namespace slp {

using std::string;
using std::vector;

SLPClient::SLPClient(ConnectedDescriptor *descriptor)
    : m_core(new SLPClientCore(descriptor)) {
}


SLPClient::~SLPClient() {}


/*
 * Setup this client
 * @returns true on success, false on failure
 */
bool SLPClient::Setup() { return m_core->Setup(); }


/*
 * Close the ola connection.
 * @return true on sucess, false on failure
 */
bool SLPClient::Stop() { return m_core->Stop(); }


/*
 * Register a service in SLP.
 * @param service the name of the service.
 * @param lifetime the lifetime of the service.
 * @returns true if the request succeeded, false otherwise.
 */
bool SLPClient::RegisterService(
    const vector<string> &scopes,
    const string &service,
    uint16_t lifetime,
    SingleUseCallback2<void, const string&, uint16_t> *callback) {
  return m_core->RegisterService(scopes, service, lifetime, callback);
}


/*
 * Register a persistent service in SLP.
 * @param service the name of the service.
 * @param lifetime the lifetime of the service.
 * @returns true if the request succeeded, false otherwise.
 */
bool SLPClient::RegisterPersistentService(
      const vector<string> &scopes,
      const string &service,
      uint16_t lifetime,
      SingleUseCallback2<void, const string&, uint16_t> *callback) {
  return m_core->RegisterPersistentService(scopes, service, lifetime, callback);
}


/**
 * DeRegister a service
 * @param service the name of the service.
 * @returns true if the request succeeded, false otherwise.
 */
bool SLPClient::DeRegisterService(
    const vector<string> &scopes,
    const string &service,
    SingleUseCallback2<void, const string&, uint16_t> *callback) {
  return m_core->DeRegisterService(scopes, service, callback);
}


/*
 * Register a persistent service in SLP.
 * @param scopes a list of scopes to search.
 * @param service the name of the service.
 * @param lifetime the lifetime of the service.
 * @returns true if the request succeeded, false otherwise.
 */
bool SLPClient::FindService(
    const vector<string> &scopes,
    const string &service,
    SingleUseCallback2<void, const string&,
                       const vector<URLEntry>&> *callback) {
  return m_core->FindService(scopes, service, callback);
}


/**
 * Get info about the server.
 * @returns true if the request succeeded, false otherwise.
 */
bool SLPClient::GetServerInfo(
    SingleUseCallback2<void, const string&, const ServerInfo&> *callback) {
  return m_core->GetServerInfo(callback);
}
}  // namespace slp
}  // namespace ola
