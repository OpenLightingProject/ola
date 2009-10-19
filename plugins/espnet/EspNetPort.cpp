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

#include <sstream>
#include <algorithm>
#include <ola/Logging.h>
#include <olad/Universe.h>

#include "EspNetPort.h"
#include "EspNetDevice.h"
#include "EspNetNode.h"

namespace ola {
namespace plugin {
namespace espnet {


bool EspNetPort::IsOutput() const {
  // odd ports are output
  return PortId() % 2;
}


string EspNetPort::Description() const {
  std::stringstream str;
  if (GetUniverse())
    str << "EspNet universe " << GetUniverse()->UniverseId();
  return str.str();
}


/*
 * Write operation
 */
bool EspNetPort::WriteDMX(const DmxBuffer &buffer) {
  if (!IsOutput() || !GetUniverse())
    return false;

  EspNetDevice *device = GetDevice();
  EspNetNode *node = device->GetNode();

  if (!node->SendDMX(EspNetUniverseId(), buffer))
    return false;
  return true;
}


/*
 * Read operation
 * @return The DmxBuffer with the new data
 */
const DmxBuffer &EspNetPort::ReadDMX() const {
  return m_buffer;
}


/*
 * Update the data buffer for this port
 */
int EspNetPort::UpdateBuffer() {
  // we can't update if this isn't a input port
  if (IsOutput() || !GetUniverse())
    return false;

  EspNetDevice *device = GetDevice();
  EspNetNode *node = device->GetNode();
  m_buffer = node->GetDMX(EspNetUniverseId());
  return DmxChanged();
}


/*
 * We intecept this to setup/remove the dmx handler
 */
bool EspNetPort::SetUniverse(Universe *universe) {
  Universe *old_universe = GetUniverse();
  Port<EspNetDevice>::SetUniverse(universe);

  if (IsOutput())
    return true;

  EspNetDevice *device = GetDevice();
  EspNetNode *node = device->GetNode();

  if (old_universe && old_universe != universe)
    node->RemoveHandler(old_universe->UniverseId() % ESPNET_MAX_UNIVERSES);
  if (universe && universe != old_universe)
    node->SetHandler(EspNetUniverseId(),
                     ola::NewClosure(this, &EspNetPort::UpdateBuffer));
  return true;
}


/*
 * return the EspNet universe ID for this port. In case we don't have a
 * universe, 0 is returned. Note that universe 0 is valid.
 */
uint8_t EspNetPort::EspNetUniverseId() const {
  if (GetUniverse())
    return GetUniverse()->UniverseId() % ESPNET_MAX_UNIVERSES;
  else
    return 0;
}

} //espnet
} //plugin
} //ola
