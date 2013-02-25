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
 * PortBroker.h
 * The port broker exists to handle the case where a port may be removed
 * while a RDM call is in flight. When the call completes, the port broker
 * will detect that the port has disconnected and not run the callback (which
 * would now point to an invalid memory location).
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLAD_PORTBROKER_H_
#define INCLUDE_OLAD_PORTBROKER_H_

#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/Callback.h>
#include <olad/Universe.h>
#include <olad/Port.h>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ola {

class PortBrokerInterface {
  public:
    PortBrokerInterface() {}
    virtual ~PortBrokerInterface() {}

    virtual void SendRDMRequest(
        const Port *port,
        Universe *universe,
        const ola::rdm::RDMRequest *request,
        ola::rdm::RDMCallback *callback) = 0;
};


class PortBroker: public PortBrokerInterface {
  public:
    PortBroker() {}
    ~PortBroker() {}

    void AddPort(const Port *port);
    void RemovePort(const Port *port);

    void SendRDMRequest(const Port *port,
                        Universe *universe,
                        const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

  private:
    PortBroker(const PortBroker&);
    PortBroker& operator=(const PortBroker&);

    typedef std::pair<string, const Port*> port_key;

    void RequestComplete(port_key key,
                         ola::rdm::RDMCallback *callback,
                         ola::rdm::rdm_response_code code,
                         const ola::rdm::RDMResponse *response,
                         const std::vector<std::string> &packets);

    std::set<port_key> m_ports;
};
}  // ola
#endif  // INCLUDE_OLAD_PORTBROKER_H_
