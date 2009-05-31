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
 * ShowNetPort.cpp
 * The ShowNet plugin for lla
 * Copyright (C) 2005-2009 Simon Newton
 */

#include "ShowNetPort.h"
#include "ShowNetDevice.h"

#include <lla/Closure.h>
#include <lla/Logging.h>
#include <llad/Universe.h>


namespace lla {
namespace shownet {


bool ShowNetPort::CanRead() const {
  // ports 0 to 7 are input
  return (PortId() < ShowNetNode::SHOWNET_MAX_UNIVERSES);
}


bool ShowNetPort::CanWrite() const {
  // ports 8 to 13 are output
  return (PortId() >= ShowNetNode::SHOWNET_MAX_UNIVERSES &&
          PortId() < 2 * ShowNetNode::SHOWNET_MAX_UNIVERSES);
}


bool ShowNetPort::WriteDMX(const DmxBuffer &buffer) {
  ShowNetDevice *device = (ShowNetDevice*) GetDevice();

  if (!CanWrite())
    return false;

  ShowNetNode *node = device->GetNode();
  if (!node->SendDMX(ShowNetUniverseId(), buffer))
    return false;
  return true;
}

const DmxBuffer &ShowNetPort::ReadDMX() const {
  return m_buffer;
}

/*
 * Called when there is new dmx data
 */
int ShowNetPort::UpdateBuffer() {
  ShowNetDevice *device = (ShowNetDevice*) GetDevice();
  ShowNetNode *node = device->GetNode();
  m_buffer = node->GetDMX(ShowNetUniverseId());
  return DmxChanged();
}

/*
 * We intecept this to setup/remove the dmx handler
 */
bool ShowNetPort::SetUniverse(Universe *universe) {
  ShowNetDevice *device = (ShowNetDevice*) GetDevice();
  ShowNetNode *node = device->GetNode();

  Universe *old_universe = GetUniverse();
  Port::SetUniverse(universe);

  if (!old_universe && universe)
    node->SetHandler(ShowNetUniverseId(),
                     lla::NewClosure(this, &ShowNetPort::UpdateBuffer));
  else if (old_universe && !universe)
    node->RemoveHandler(ShowNetUniverseId());
  return true;
}

} //plugin
} //lla
