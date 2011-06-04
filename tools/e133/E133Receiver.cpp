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
 * E133Receiver.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMHelper.h>

#include <string>
#include <vector>

#include "plugins/e131/e131/E133Header.h"

#include "E133Receiver.h"

using ola::plugin::e131::E133Header;
using ola::rdm::RDMRequest;


E133Receiver::E133Receiver(unsigned int universe,
                           ola::rdm::RDMControllerInterface *local_controller)
    : m_e133_layer(NULL),
      m_local_controller(local_controller),
      m_universe(universe) {
  if (!universe)
    OLA_FATAL <<
      "E133Receiver created with universe 0, this isn't valid";
}


/**
 * Check for requests that have timed out
 */
void E133Receiver::CheckForStaleRequests(const ola::TimeStamp *now) {
  (void) now;
}


/**
 * Handle a RDM response addressed to this universe
 */
void E133Receiver::HandlePacket(
    const ola::plugin::e131::TransportHeader &transport_header,
    const ola::plugin::e131::E133Header &e133_header,
    const std::string &raw_request) {
  OLA_INFO << "Got data from " << transport_header.SourceIP() <<
    " for universe " << e133_header.Universe();

  // attempt to unpack as a request
  const ola::rdm::RDMRequest *request = ola::rdm::RDMRequest::InflateFromData(
    reinterpret_cast<const uint8_t*>(raw_request.data()),
    raw_request.size());

  if (!request) {
    OLA_WARN << "Failed to unpack E1.33 RDM message, ignoring request.";
    return;
  }

  m_local_controller->SendRDMRequest(
      request,
      ola::NewSingleCallback(this, &E133Receiver::RequestComplete));
}


/**
 * Handle a completed RDM request
 */
void E133Receiver::RequestComplete(ola::rdm::rdm_response_code response_code,
                                   const ola::rdm::RDMResponse *response,
                                   const std::vector<std::string> &packets) {
  if (response_code != ola::rdm::RDM_COMPLETED_OK) {
    OLA_WARN << "E1.33 request failed with code " <<
      ola::rdm::ResponseCodeToString(response_code) <<
      ", dropping request";
  }

  OLA_INFO << "we'd send the response here";

  // send response back to src ip:port with correct seq #
  (void) packets;

  if (response)
    delete response;
}
