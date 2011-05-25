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

#include <ola/Logging.h>
#include <ola/rdm/RDMCommand.h>

#include <string>

#include "plugins/e131/e131/E133Header.h"

#include "E133Receiver.h"

using ola::plugin::e131::E133Header;
using ola::rdm::RDMRequest;


E133Receiver::E133Receiver(unsigned int universe)
    : m_e133_layer(NULL),
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
    const std::string &raw_response) {
  OLA_INFO << "Got data from " << transport_header.SourceIP() <<
    " for universe " << e133_header.Universe();
  (void) raw_response;
}
