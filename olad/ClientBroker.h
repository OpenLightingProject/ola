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
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/Callback.h"
#include "olad/Universe.h"
#include "olad/Client.h"

namespace ola {

class ClientBroker {
  public:
    ClientBroker() {}
    ~ClientBroker() {}

    void AddClient(const Client *client);
    void RemoveClient(const Client *client);

    void SendRDMRequest(const Client *client,
                        Universe *universe,
                        const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

  private:
    ClientBroker(const ClientBroker&);
    ClientBroker& operator=(const ClientBroker&);

    void RequestComplete(const void *key,
                         ola::rdm::RDMCallback *callback,
                         ola::rdm::rdm_response_status status,
                         const ola::rdm::RDMResponse *response);

    std::set<const void*> m_clients;
};
}  // ola
#endif  // OLAD_CLIENTBROKER_H_
