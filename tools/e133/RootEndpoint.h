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
 * RootEndpoint.h
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/rdm/UID.h>

#include "tools/e133/E133Endpoint.h"

#ifndef TOOLS_E133_ROOTENDPOINT_H_
#define TOOLS_E133_ROOTENDPOINT_H_

using ola::rdm::RDMRequest;
using ola::rdm::RDMCallback;

/**
 * The root endpoint responsible for handling PIDs defined in E1.33.
 */
class RootEndpoint: public E133EndpointInterface {
  public:
    explicit RootEndpoint(const ola::rdm::UID &uid,
                          const class EndpointManager *endpoint_manager,
                          class TCPConnectionStats *tcp_stats);
    ~RootEndpoint() {}

    void SendRDMRequest(const RDMRequest *request,
                        RDMCallback *on_complete);

  private:
    ola::rdm::UID m_uid;
    const class EndpointManager *m_endpoint_manager;
    class TCPConnectionStats *m_tcp_stats;

    void HandleSupportedParams(const RDMRequest *request,
                               RDMCallback *on_complete);
    void HandleEndpointList(const RDMRequest *request,
                            RDMCallback *on_complete);
    void HandleEndpointIdentify(const RDMRequest *request,
                                RDMCallback *on_complete);
    void HandleEndpointLabel(const RDMRequest *request,
                             RDMCallback *on_complete);
    void HandleTCPCommsStatus(const RDMRequest *request,
                              RDMCallback *on_complete);
    void HandleUnknownPID(const RDMRequest *request,
                          RDMCallback *on_complete);

    bool SanityCheckGet(const RDMRequest *request,
                        RDMCallback *callback,
                        unsigned int data_length);
    bool SanityCheckGetOrSet(const RDMRequest *request,
                             RDMCallback *callback,
                             unsigned int get_length,
                             unsigned int min_set_length,
                             unsigned int max_set_length);
    void RunRDMCallback(RDMCallback *callback,
                        ola::rdm::RDMResponse *response);
};
#endif  // TOOLS_E133_ROOTENDPOINT_H_
