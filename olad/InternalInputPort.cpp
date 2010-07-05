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
 * InternalInputPort.cpp
 * An input port used to send RDM commands to a universe
 * Copyright (C) 2010 Simon Newton
 */

#include <string>
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "olad/InternalInputPort.h"

namespace ola {

/*
 * This port never generates data so this shouldn't ever be called
 */
const DmxBuffer& InternalInputPort::ReadDMX() const {
  OLA_WARN << "Attempt to read DMX from an Internal Port!";
  return m_buffer;
}


/*
 * Handle an RDM response, this passes it on to the
 * InternalInputPortResponseHandler
 */
bool InternalInputPort::HandleRDMResponse(
    const ola::rdm::RDMResponse *response) {
  if (m_handler) {
    if (GetUniverse())
      return m_handler->HandleRDMResponse(GetUniverse()->UniverseId(),
                                          response);
    else
      OLA_WARN << "No universe for an internal port " << PortId();
  } else {
      OLA_WARN << "No handler for internal port " << PortId();
  }
  delete response;
  return false;
}


string InternalInputPort::UniqueId() const {
  std::stringstream str;
  str << "internal-I-" << PortId();
  return str.str();
}
}  // ola
