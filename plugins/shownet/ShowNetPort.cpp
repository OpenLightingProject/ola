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
 * The ShowNet plugin for ola
 * Copyright (C) 2005-2009 Simon Newton
 */
#include <sstream>
#include <string>

#include "ola/BaseTypes.h"
#include "ola/Closure.h"
#include "ola/Logging.h"
#include "plugins/shownet/ShowNetDevice.h"
#include "plugins/shownet/ShowNetPort.h"

namespace ola {
namespace plugin {
namespace shownet {

bool ShowNetPort::IsOutput() const {
  // odd ports are output
  return (PortId() % 2);
}


string ShowNetPort::Description() const {
  std::stringstream str;
  str << "ShowNet " << ShowNetUniverseId() * DMX_UNIVERSE_SIZE + 1 << "-" <<
    (ShowNetUniverseId() + 1) * DMX_UNIVERSE_SIZE;
  return str.str();
}


bool ShowNetPort::WriteDMX(const DmxBuffer &buffer) {
  ShowNetDevice *device = GetDevice();

  if (!IsOutput())
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
  ShowNetDevice *device = GetDevice();
  ShowNetNode *node = device->GetNode();
  m_buffer = node->GetDMX(ShowNetUniverseId());
  return DmxChanged();
}


/*
 * We intecept this to setup/remove the dmx handler
 */
bool ShowNetPort::SetUniverse(Universe *universe) {
  Universe *old_universe = GetUniverse();
  Port<ShowNetDevice>::SetUniverse(universe);

  if (IsOutput())
    return true;

  ShowNetDevice *device = GetDevice();
  ShowNetNode *node = device->GetNode();

  if (!old_universe && universe)
    node->SetHandler(ShowNetUniverseId(),
                     ola::NewClosure(this, &ShowNetPort::UpdateBuffer));
  else if (old_universe && !universe)
    node->RemoveHandler(ShowNetUniverseId());
  return true;
}
}  // shownet
}  // plugin
}  // ola
