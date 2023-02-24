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
 * PortBroker.h
 * The port broker exists to handle the case where a port may be removed
 * while a RDM call is in flight. When the call completes, the port broker
 * will detect that the port has disconnected and not run the callback (which
 * would now point to an invalid memory location).
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLAD_PORTBROKER_H_
#define INCLUDE_OLAD_PORTBROKER_H_

#include <ola/base/Macro.h>
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
        ola::rdm::RDMRequest *request,
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
                        ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

 private:
    typedef std::pair<std::string, const Port*> port_key;

    void RequestComplete(port_key key,
                         ola::rdm::RDMCallback *callback,
                         ola::rdm::RDMReply *reply);

    std::set<port_key> m_ports;

    DISALLOW_COPY_AND_ASSIGN(PortBroker);
};
}  // namespace ola
#endif  // INCLUDE_OLAD_PORTBROKER_H_
