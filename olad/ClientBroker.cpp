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
 * ClientBroker.cpp
 * Acts as the glue between clients and the RDM request path.
 * Copyright (C) 2010 Simon Newton
 */

#include <set>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "olad/ClientBroker.h"

namespace ola {

using std::set;
using std::string;
using std::vector;

void ClientBroker::AddClient(const Client *client) {
  m_clients.insert(client);
}

void ClientBroker::RemoveClient(const Client *client) {
  m_clients.erase(client);
}

void ClientBroker::SendRDMRequest(const Client *client,
                                  Universe *universe,
                                  ola::rdm::RDMRequest *request,
                                  ola::rdm::RDMCallback *callback) {
  if (!STLContains(m_clients, client)) {
    OLA_WARN << "Making an RDM call but the client doesn't exist in the "
             << "broker!";
  }

  universe->SendRDMRequest(
      request,
      NewSingleCallback(this, &ClientBroker::RequestComplete, client,
                        callback));
}

void ClientBroker::RunRDMDiscovery(const Client *client,
                                   Universe *universe,
                                   bool full_discovery,
                                   ola::rdm::RDMDiscoveryCallback *callback) {
  if (!STLContains(m_clients, client)) {
    OLA_WARN << "Running RDM discovery but the client doesn't exist in the "
             << "broker!";
  }

  universe->RunRDMDiscovery(
      NewSingleCallback(this, &ClientBroker::DiscoveryComplete, client,
                        callback),
      full_discovery);
}

/*
 * Return from an RDM call.
 * @param key the client associated with this request
 * @param callback the callback to run if the key still exists
 * @param code the code of the RDM request
 * @param response the RDM response
 */
void ClientBroker::RequestComplete(const Client *client,
                                   ola::rdm::RDMCallback *callback,
                                   ola::rdm::RDMReply *reply) {
  if (!STLContains(m_clients, client)) {
    OLA_DEBUG << "Client no longer exists, cleaning up from RDM response";
    delete callback;
  } else {
    callback->Run(reply);
  }
}

void ClientBroker::DiscoveryComplete(
    const Client *client,
    ola::rdm::RDMDiscoveryCallback *callback,
    const ola::rdm::UIDSet &uids) {
  if (!STLContains(m_clients, client)) {
    OLA_DEBUG << "Client no longer exists, cleaning up from RDM discovery";
    delete callback;
  } else {
    callback->Run(uids);
  }
}
}  // namespace ola
