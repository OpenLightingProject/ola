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
 * E133Endpoint.h
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <string>
#include "ola/rdm/RDMControllerInterface.h"

#ifndef TOOLS_E133_E133ENDPOINT_H_
#define TOOLS_E133_E133ENDPOINT_H_

using std::string;


static const uint16_t ROOT_E133_ENDPOINT = 0;

/**
 * The base class for E1.33 Endpoints.
 * Endpoints are tasked with handling RDM requests.
 */
class E133EndpointInterface: public ola::rdm::RDMControllerInterface {
  public:
    E133EndpointInterface() {}
    virtual ~E133EndpointInterface() {}
};


/**
 * A non-root endpoint, which has properties like a label, identify mode etc.
 */
class E133Endpoint: public E133EndpointInterface,
                           ola::rdm::DiscoverableRDMControllerInterface {
  public:
    explicit E133Endpoint(DiscoverableRDMControllerInterface *controller);
    ~E133Endpoint() {}

    bool IdentifyMode() const { return m_identify_mode; }
    void SetIdentifyMode(bool identify_on) { m_identify_mode = identify_on; }

    uint16_t Universe() const { return m_universe; }
    void SetUniverse(uint16_t universe) { m_universe = universe; }

    string Label() const { return m_endpoint_label; }
    void SetLabel(const string &endpoint_label) {
      m_endpoint_label = endpoint_label;
    }

    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *on_complete);

  private:
    bool m_identify_mode;
    uint16_t m_universe;
    string m_endpoint_label;
    DiscoverableRDMControllerInterface *m_controller;
};
#endif  // TOOLS_E133_E133ENDPOINT_H_
