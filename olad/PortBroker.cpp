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
 * PortBroker.cpp
 * Acts as the glue between ports and the RDM request path.
 * Copyright (C) 2010 Simon Newton
 */

#include <set>
#include <string>
#include <vector>
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "olad/PortBroker.h"

namespace ola {

using std::set;
using std::pair;

/**
 * Add a port to the broker
 */
void PortBroker::AddPort(const Port *port) {
  port_key key(
      port->UniqueId(),
      reinterpret_cast<const void*>(port));
  m_ports.insert(key);
}


/**
 * Remove a port from the broker
 */
void PortBroker::RemovePort(const Port *port) {
  port_key key(
      port->UniqueId(),
      reinterpret_cast<const void*>(port));
  m_ports.erase(key);
}


/**
 * Make an RDM call
 * @param port the OlaPortService that should exist when the call returns
 * @param universe the universe to send the RDM request on
 * @param request the RDM request
 * @param callback the callback to run when the request completes
 */
void PortBroker::SendRDMRequest(const Port *port,
                                Universe *universe,
                                const ola::rdm::RDMRequest *request,
                                ola::rdm::RDMCallback *callback) {
  port_key key(
      port->UniqueId(),
      reinterpret_cast<const void*>(port));
  set<port_key>::const_iterator iter = m_ports.find(key);
  if (iter == m_ports.end())
    OLA_WARN <<
      "Making an RDM call but the port doesn't exist in the broker!";

  universe->SendRDMRequest(request,
      NewCallback(this,
                  &PortBroker::RequestComplete,
                  key,
                  callback));
}


/**
 * Return from an RDM call
 * @param key the port associated with this request
 * @param callback the callback to run if the key still exists
 * @param status the status of the RDM request
 * @param response the RDM response
 */
void PortBroker::RequestComplete(port_key key,
                                 ola::rdm::RDMCallback *callback,
                                 ola::rdm::rdm_response_code code,
                                 const ola::rdm::RDMResponse *response,
                                 const std::vector<std::string> &packets) {
  set<port_key>::const_iterator iter = m_ports.find(key);
  if (iter == m_ports.end()) {
    OLA_INFO << "Port no longer exists, cleaning up from RDM response";
    delete response;
    delete callback;
  } else {
    callback->Run(code, response, packets);
  }
}
}  // ola
