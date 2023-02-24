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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * SandNetPort.cpp
 * The SandNet plugin for ola
 * Copyright (C) 2005 Simon Newton
 */

#include <algorithm>
#include <sstream>
#include <string>
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "olad/Universe.h"

#include "plugins/sandnet/SandNetPort.h"
#include "plugins/sandnet/SandNetDevice.h"
#include "plugins/sandnet/SandNetCommon.h"

namespace ola {
namespace plugin {
namespace sandnet {

using std::string;

// We override the set universe method to update the universe -> port hash
bool SandNetPortHelper::PreSetUniverse(Universe *old_universe,
                                       Universe *new_universe) {
  if (new_universe && !new_universe->UniverseId()) {
      OLA_WARN << "Can't use universe 0 with Sandnet!";
      return false;
  }
  (void) old_universe;
  return true;
}


string SandNetPortHelper::Description(const Universe *universe) const {
  std::ostringstream str;
  if (universe) {
    str << "Sandnet group " << static_cast<int>(SandnetGroup(universe)) <<
      ", universe " << 1 + SandnetUniverse(universe);
  }
  return str.str();
}

uint8_t SandNetPortHelper::SandnetGroup(const Universe *universe) const {
  if (universe)
    return (uint8_t) ((universe->UniverseId() - 1) >> 8);
  return 0;
}

uint8_t SandNetPortHelper::SandnetUniverse(const Universe *universe) const {
  if (universe)
    return universe->UniverseId() - 1;
  return 0;
}

void SandNetInputPort::PostSetUniverse(Universe *old_universe,
                                       Universe *new_universe) {
  if (old_universe)
    m_node->RemoveHandler(m_helper.SandnetGroup(old_universe),
                          m_helper.SandnetUniverse(old_universe));

  if (new_universe) {
    m_node->SetHandler(
        m_helper.SandnetGroup(new_universe),
        m_helper.SandnetUniverse(new_universe),
        &m_buffer,
        NewCallback<SandNetInputPort, void>(this,
                                           &SandNetInputPort::DmxChanged));
  }
}


bool SandNetOutputPort::WriteDMX(const DmxBuffer &buffer,
                                 uint8_t priority) {
  (void) priority;
  if (!GetUniverse())
    return false;

  if (!m_node->SendDMX(PortId(), buffer))
    return false;
  return true;
}


void SandNetOutputPort::PostSetUniverse(Universe *old_universe,
                                        Universe *new_universe) {
  if (new_universe)
    m_node->SetPortParameters(PortId(),
                              SandNetNode::SANDNET_PORT_MODE_IN,
                              m_helper.SandnetGroup(new_universe),
                              m_helper.SandnetUniverse(new_universe));
  (void) old_universe;
}
}  // namespace sandnet
}  // namespace plugin
}  // namespace ola
