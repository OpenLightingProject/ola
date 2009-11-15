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
 *
 * SandNetPort.cpp
 * The SandNet plugin for ola
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include "ola/Closure.h"
#include "ola/Logging.h"
#include "olad/Universe.h"

#include "plugins/sandnet/SandNetPort.h"
#include "plugins/sandnet/SandNetDevice.h"
#include "plugins/sandnet/SandNetCommon.h"

namespace ola {
namespace plugin {
namespace sandnet {


bool SandNetPort::IsOutput() const {
  // ports 0 & 1 are output (sandnet only allows 2 output ports per device)
  return PortId() < SANDNET_MAX_PORTS;
}


string SandNetPort::Description() const {
  std::stringstream str;
  if (GetUniverse()) {
    str << "Sandnet group " << static_cast<int>(SandnetGroup(GetUniverse())) <<
      ", universe " << 1 + SandnetUniverse(GetUniverse());
  }
  return str.str();
}


/*
 * Write operation
 */
bool SandNetPort::WriteDMX(const DmxBuffer &buffer) {
  if (!IsOutput() || !GetUniverse())
    return false;

  SandNetNode *node = GetDevice()->GetNode();

  if (!node->SendDMX(PortId(), buffer))
    return false;
  return true;
}


/*
 * Update the data buffer for this port
 */
int SandNetPort::UpdateBuffer() {
  // we can't update if this isn't a input port
  if (IsOutput() || !GetUniverse())
    return false;

  SandNetNode *node = GetDevice()->GetNode();
  m_buffer = node->GetDMX(SandnetGroup(GetUniverse()),
                          SandnetUniverse(GetUniverse()));
  return DmxChanged();
}


/*
 * We override the set universe method to update the universe -> port hash
 */
bool SandNetPort::SetUniverse(Universe *universe) {
  SandNetDevice *device = GetDevice();
  SandNetNode *node = device->GetNode();

  // TODO(simon): move this into the base class
  if (universe) {
    vector<AbstractPort*> ports = device->Ports();
    vector<AbstractPort*>::const_iterator iter;
    for (iter = ports.begin(); iter != ports.end(); ++iter) {
      if ((*iter)->IsOutput() == IsOutput() &&
          (*iter)->GetUniverse() &&
          (*(*iter)->GetUniverse()) == *universe) {
        OLA_WARN << "Port " << (*iter)->PortId() <<
          " is already patched to universe " << universe->UniverseId();
        return false;
      }
    }
  }

  if (IsOutput()) {
    if (universe) {
      if (!universe->UniverseId()) {
        OLA_WARN << "Can't use universe 0 with Sandnet!";
        return false;
      }
      node->SetPortParameters(PortId(),
                              SandNetNode::SANDNET_PORT_MODE_IN,
                              SandnetGroup(universe),
                              SandnetUniverse(universe));
    }
  } else {
    Universe *old_universe = GetUniverse();
    if (old_universe)
      node->RemoveHandler(SandnetGroup(old_universe),
                          SandnetUniverse(old_universe));

    if (universe) {
      node->SetHandler(SandnetGroup(universe),
                       SandnetUniverse(universe),
                       NewClosure(this, &SandNetPort::UpdateBuffer));
    }
  }
  return Port<SandNetDevice>::SetUniverse(universe);
}


/*
 * Return the sandnet group that corresponds to a OLA Universe.
 * @param universe the OLA universe
 * @returns the sandnet group number
 */
uint8_t SandNetPort::SandnetGroup(const Universe *universe) const {
  if (universe)
    return (uint8_t) ((universe->UniverseId() - 1) >> 8);
  return 0;
}


/*
 * Return the sandnet group that corresponds to a OLA Universe. Sandnet
 * Universes range from 0 to 255 (represented as 1 to 256 in the packets).
 * @param universe the OLA universe
 * @returns the sandnet universe number
 */
uint8_t SandNetPort::SandnetUniverse(const Universe *universe) const {
  if (universe)
    return universe->UniverseId() - 1;
  return 0;
}

}  // sandnet
}  // plugin
}  // ola
