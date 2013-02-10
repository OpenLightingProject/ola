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
 * E133Endpoint.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/rdm/UID.h>
#include <ola/rdm/UIDSet.h>
#include <memory>
#include <string>
#include <vector>
#include "tools/e133/E133Endpoint.h"

using ola::rdm::RDMCallback;
using ola::rdm::RDMRequest;
using std::auto_ptr;
using std::string;
using ola::rdm::RDMDiscoveryCallback;

typedef std::vector<std::string> RDMPackets;


E133Endpoint::E133Endpoint(DiscoverableRDMControllerInterface *controller)
    : m_identify_mode(false),
      m_universe(0),
      m_endpoint_label(""),
      m_controller(controller) {
}


void E133Endpoint::SetIdentifyMode(bool identify_on) {
  m_identify_mode = identify_on;
  OLA_INFO << "IDENTIFY MODE " << (identify_on ? "ON" : "OFF");
}

/**
 * Run full discovery for this endpoint
 */
void E133Endpoint::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  if (m_controller) {
    m_controller->RunFullDiscovery(callback);
    return;
  }

  ola::rdm::UIDSet uid_set;
  callback->Run(uid_set);
}


/**
 * Run incremental discovery for this endpoint
 */
void E133Endpoint::RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
  if (m_controller) {
    m_controller->RunIncrementalDiscovery(callback);
    return;
  }

  ola::rdm::UIDSet uid_set;
  callback->Run(uid_set);
}


/**
 * Handle RDM requests to this endpoint
 */
void E133Endpoint::SendRDMRequest(const RDMRequest *request_ptr,
                                  RDMCallback *on_complete) {
  auto_ptr<const RDMRequest> request(request_ptr);

  // for now just fail all requests
  OLA_WARN << "Received request to endpoint " << m_endpoint_label;
  RDMPackets packets;
  on_complete->Run(ola::rdm::RDM_UNKNOWN_UID, NULL, packets);
}
