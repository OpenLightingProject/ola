/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ClientBroker.cpp
 * Acts as the glue between clients and the RDM request path.
 * Copyright (C) 2010 Simon Newton
 */

#include <set>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "olad/ClientBroker.h"

namespace ola {

using std::set;

/**
 * Add a client to the broker
 */
void ClientBroker::AddClient(const Client *client) {
  m_clients.insert(client);
}


/**
 * Remove a client from the broker
 */
void ClientBroker::RemoveClient(const Client *client) {
  m_clients.erase(client);
}


/**
 * Make an RDM call
 * @param client the OlaClientService that should exist when the call returns
 * @param universe the universe to send the RDM request on
 * @param request the RDM request
 * @param callback the callback to run when the request completes
 */
void ClientBroker::SendRDMRequest(const Client *client,
                                  Universe *universe,
                                  const ola::rdm::RDMRequest *request,
                                  ola::rdm::RDMCallback *callback) {
  client_set::const_iterator iter = m_clients.find(client);
  if (iter == m_clients.end())
    OLA_WARN <<
      "Making an RDM call but the client doesn't exist in the broker!";

  universe->SendRDMRequest(request,
      NewSingleCallback(this,
                        &ClientBroker::RequestComplete,
                        client,
                        callback));
}


/**
 * Return from an RDM call
 * @param key the client associated with this request
 * @param callback the callback to run if the key still exists
 * @param code the code of the RDM request
 * @param response the RDM response
 */
void ClientBroker::RequestComplete(const Client *key,
                                   ola::rdm::RDMCallback *callback,
                                   ola::rdm::rdm_response_code code,
                                   const ola::rdm::RDMResponse *response,
                                   const std::vector<std::string> &packets) {
  client_set::const_iterator iter = m_clients.find(key);
  if (iter == m_clients.end()) {
    OLA_INFO << "Client no longer exists, cleaning up from RDM response";
    delete response;
    delete callback;
  } else {
    callback->Run(code, response, packets);
  }
}
}  // ola
