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
 * EspNetPort.cpp
 * The EspNet plugin for ola
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <algorithm>
#include <sstream>
#include <string>
#include "ola/Logging.h"
#include "olad/Universe.h"
#include "plugins/espnet/EspNetPort.h"
#include "plugins/espnet/EspNetDevice.h"
#include "plugins/espnet/EspNetNode.h"

namespace ola {
namespace plugin {
namespace espnet {


string EspNetPortHelper::Description(Universe *universe) const {
  std::stringstream str;
  if (universe)
    str << "EspNet universe " << (unsigned int) EspNetUniverseId(universe);
  return str.str();
}


/*
 * Return the EspNet universe ID for this port. In case we don't have a
 * universe, 0 is returned. Note that universe 0 is valid.
 */
uint8_t EspNetPortHelper::EspNetUniverseId(Universe *universe) const {
  if (universe)
    return universe->UniverseId() % ESPNET_MAX_UNIVERSES;
  else
    return 0;
}


/*
 * Set the universe for an InputPort.
 */
void EspNetInputPort::PostSetUniverse(Universe *old_universe,
                                      Universe *new_universe) {
  if (old_universe)
    m_node->RemoveHandler(m_helper.EspNetUniverseId(old_universe));

  if (new_universe)
    m_node->SetHandler(
        m_helper.EspNetUniverseId(new_universe),
        &m_buffer,
        ola::NewClosure<EspNetInputPort>(this, &EspNetInputPort::DmxChanged));
}


/*
 * Write data to this port.
 */
bool EspNetOutputPort::WriteDMX(const DmxBuffer &buffer,
                                uint8_t priority) {
  if (!GetUniverse())
    return false;

  if (!m_node->SendDMX(m_helper.EspNetUniverseId(GetUniverse()), buffer))
    return false;
  return true;
}
}  // espnet
}  // plugin
}  // ola
