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
 * ClientBroker.h
 * The client broker exists to handle the case where a client may disconnect
 * while a RDM call is in flight. When the call completes, the client broker
 * will detect that the client has disconnected and not run the callback (which
 * would now point to an invalid memory location).
 * Copyright (C) 2010 Simon Newton
 */

#ifndef OLAD_CLIENTBROKER_H_
#define OLAD_CLIENTBROKER_H_

#include <set>
#include <string>
#include <vector>
#include "ola/base/Macro.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/Callback.h"
#include "olad/Universe.h"
#include "olad/plugin_api/Client.h"

namespace ola {

/**
 * @brief Handles async client operations.
 *
 * Since some client operations such as RDM commands are asynchronous,
 * we can run into problems if the client disconnects while the operation
 * is in progress. This is because the completion callback will hold a pointer
 * to a client which has been deleted.
 *
 * The ClientBroker acts as an in-between by holding a list of active clients
 * and proxying RDM calls. When the RDM call returns, if the client responsible
 * for the call has been deleted, we delete the callback rather then executing
 * it.
 */
class ClientBroker {
 public:
  ClientBroker() {}
  ~ClientBroker() {}

  /**
  * @brief Add a client to the broker.
  * @param client the Client to add. Ownership is not transferred.
  */
  void AddClient(const Client *client);

  /**
   * @brief Remove a client from the broker.
   * @param client The Client to remove.
   */
  void RemoveClient(const Client *client);

  /**
   * @brief Make an RDM call.
   * @param client the Client responsible for making the call.
   * @param universe the universe to send the RDM request on.
   * @param request the RDM request.
   * @param callback the callback to run when the request completes. Ownership
   *   is transferred.
   */
  void SendRDMRequest(const Client *client,
                      Universe *universe,
                      ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *callback);

  /**
   * @brief Make an RDM call.
   * @param client the Client responsible for making the call.
   * @param universe the universe to send the RDM request on.
   * @param full_discovery true for full discovery, false for incremental.
   * @param callback the callback to run when the request completes. Ownership
   *   is transferred.
   */
  void RunRDMDiscovery(const Client *client,
                       Universe *universe,
                       bool full_discovery,
                       ola::rdm::RDMDiscoveryCallback *callback);

 private:
  typedef std::set<const Client*> client_set;

  client_set m_clients;

  void RequestComplete(const Client *key,
                       ola::rdm::RDMCallback *callback,
                       ola::rdm::RDMReply *reply);

  void DiscoveryComplete(const Client *key,
                         ola::rdm::RDMDiscoveryCallback *on_complete,
                         const ola::rdm::UIDSet &uids);

  DISALLOW_COPY_AND_ASSIGN(ClientBroker);
};
}  // namespace ola
#endif  // OLAD_CLIENTBROKER_H_
