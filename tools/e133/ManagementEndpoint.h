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
 * ManagementEndpoint.h
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/rdm/UID.h>

#include "tools/e133/E133Endpoint.h"
#include "ola/rdm/ResponderOps.h"

#ifndef TOOLS_E133_MANAGEMENTENDPOINT_H_
#define TOOLS_E133_MANAGEMENTENDPOINT_H_

using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMCallback;

/**
 * The ManagementEndpoint handles RDMCommands directed at this E1.33 Component. It
 * can also pass through commands to another controller if there is one
 * supplied.
 */
class ManagementEndpoint: public E133Endpoint {
 public:
    ManagementEndpoint(DiscoverableRDMControllerInterface *controller,
                       const EndpointProperties &properties,
                       const ola::rdm::UID &uid,
                       const class EndpointManager *endpoint_manager,
                       class TCPConnectionStats *tcp_stats);
    ~ManagementEndpoint() {}

    void SendRDMRequest(RDMRequest *request, RDMCallback *on_complete);

    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

 private:
    /**
     * The RDM Operations for the ManagementEndpoint.
     */
    class RDMOps : public ola::rdm::ResponderOps<ManagementEndpoint> {
     public:
      static RDMOps *Instance() {
        if (!instance)
          instance = new RDMOps();
        return instance;
      }

     private:
      RDMOps() : ola::rdm::ResponderOps<ManagementEndpoint>(PARAM_HANDLERS) {}

      static RDMOps *instance;
    };

    const ola::rdm::UID m_uid;
    const class EndpointManager *m_endpoint_manager;
    class TCPConnectionStats *m_tcp_stats;
    DiscoverableRDMControllerInterface *m_controller;

    // RDM PID handlers.
    RDMResponse *GetEndpointList(const RDMRequest *request);
    RDMResponse *GetEndpointListChange(const RDMRequest *request);
    RDMResponse *GetIdentifyEndpoint(const RDMRequest *request);
    RDMResponse *SetIdentifyEndpoint(const RDMRequest *request);
    RDMResponse *GetEndpointToUniverse(const RDMRequest *request);
    RDMResponse *SetEndpointToUniverse(const RDMRequest *request);
    RDMResponse *GetEndpointMode(const RDMRequest *request);
    RDMResponse *SetEndpointMode(const RDMRequest *request);
    RDMResponse *GetEndpointLabel(const RDMRequest *request);
    RDMResponse *SetEndpointLabel(const RDMRequest *request);
    RDMResponse *GetEndpointResponderListChange(const RDMRequest *request);
    RDMResponse *GetEndpointResponders(const RDMRequest *request);
    RDMResponse *GetTCPCommsStatus(const RDMRequest *request);
    RDMResponse *SetTCPCommsStatus(const RDMRequest *request);

    void DiscoveryComplete(ola::rdm::RDMDiscoveryCallback *callback,
                           const ola::rdm::UIDSet &uids);

    static const ola::rdm::ResponderOps<ManagementEndpoint>::ParamHandler
      PARAM_HANDLERS[];
};
#endif  // TOOLS_E133_MANAGEMENTENDPOINT_H_
