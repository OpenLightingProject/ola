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
 * SLPClient.cpp
 * Implementation of SLPClient
 * Copyright (C) 2012 Simon Newton
 */

#include <string>
#include <vector>

#include "ola/Callback.h"
#include "SLPClient.h"
#include "SLPClientCore.h"

namespace ola {
namespace slp {

using std::string;
using std::vector;

SLPClient::SLPClient(ConnectedDescriptor *descriptor)
    : m_core(new SLPClientCore(descriptor)) {
}


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
    const string &service,
    uint16_t lifetime,
    SingleUseCallback2<void, const string&, uint16_t> *callback) {
  return m_core->RegisterService(service, lifetime, callback);
}


/*
 * Register a persistent service in SLP.
 * @param service the name of the service.
 * @param lifetime the lifetime of the service.
 * @returns true if the request succeeded, false otherwise.
 */
bool SLPClient::RegisterPersistentService(
      const string &service,
      uint16_t lifetime,
      SingleUseCallback2<void, const string&, uint16_t> *callback) {
  return m_core->RegisterPersistentService(service, lifetime, callback);
}


/*
 * Register a persistent service in SLP.
 * @param service the name of the service.
 * @param lifetime the lifetime of the service.
 * @returns true if the request succeeded, false otherwise.
 */
bool SLPClient::FindService(
        const string &service,
        SingleUseCallback2<void, const string&,
                           const vector<SLPService>&> *callback) {
  return m_core->FindService(service, callback);
}
}  // slp
}  // ola
