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
 * RootEndpoint.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/rdm/UID.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMResponseCodes.h>

#include <memory>
#include <string>
#include <vector>

#include "tools/e133/RootEndpoint.h"

using ola::rdm::RDMCallback;
using ola::rdm::RDMRequest;
using std::auto_ptr;


typedef std::vector<std::string> RDMPackets;

RootEndpoint::RootEndpoint(const ola::rdm::UID &uid)
    : m_uid(uid) {
}

void RootEndpoint::SendRDMRequest(const RDMRequest *request_ptr,
                                  RDMCallback *on_complete) {
  auto_ptr<const RDMRequest> request(request_ptr);

  if (request->DestinationUID() != m_uid) {
    OLA_WARN << "Got a request to the root endpoint for the incorrect UID." <<
      "Expected " << m_uid << ", got " << request->DestinationUID();
    RDMPackets packets;
    on_complete->Run(ola::rdm::RDM_UNKNOWN_UID, NULL, packets);
    return;
  }

  OLA_INFO << "Received Request for root endpoint";

  // TODO(simon): switch based on PID here
  // on_complete->Run();
}
