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
 * E131Port.cpp
 * The E1.31 plugin for ola
 * Copyright (C) 2007-2009 Simon Newton
 */

#include <string>
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "olad/Universe.h"
#include "plugins/e131/E131Port.h"
#include "plugins/e131/E131Device.h"


namespace ola {
namespace plugin {
namespace e131 {


bool E131PortHelper::PreSetUniverse(Universe *new_universe,
                                    Universe *old_universe) {
  if (new_universe && (new_universe->UniverseId() == 0 ||
                       new_universe->UniverseId() > MAX_E131_UNIVERSE)) {
    OLA_WARN << "Universe id " << new_universe->UniverseId() << " is 0 or > "
      << MAX_E131_UNIVERSE;
    return false;
  }
  (void) old_universe;
  return true;
}


string E131PortHelper::Description(Universe *universe) const {
  std::stringstream str;
  if (universe)
    str << "E1.31 Universe " << universe->UniverseId();
  return str.str();
}



/*
 * Set the universe for an input port.
 */
void E131InputPort::PostSetUniverse(Universe *new_universe,
                                    Universe *old_universe) {
  if (old_universe)
    m_node->RemoveHandler(old_universe->UniverseId());

  if (new_universe)
    m_node->SetHandler(
        new_universe->UniverseId(),
        &m_buffer,
        NewClosure<E131InputPort>(this, &E131InputPort::DmxChanged));
}


/*
 * Set the universe for an output port.
 */
void E131OutputPort::PostSetUniverse(Universe *new_universe,
                                     Universe *old_universe) {
  if (new_universe) {
    if (m_prepend_hostname) {
      std::stringstream str;
      str << ola::network::Hostname() << "-" << new_universe->Name();
      m_node->SetSourceName(new_universe->UniverseId(), str.str());
    } else {
      m_node->SetSourceName(new_universe->UniverseId(), new_universe->Name());
    }
  } else {
    m_node->SetSourceName(old_universe->UniverseId(), "");
  }
}


/*
 * Write data to this port.
 */
bool E131OutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
  Universe *universe = GetUniverse();
  if (!universe)
    return false;

  if (GetPriorityMode() == PRIORITY_MODE_OVERRIDE)
    priority = GetPriority();

  return m_node->SendDMX(universe->UniverseId(),
                         buffer,
                         priority,
                         m_preview_on);
}


/*
 * Update the universe name
 */
void E131OutputPort::UniverseNameChanged(const string &new_name) {
  m_node->SetSourceName(GetUniverse()->UniverseId(), new_name);
}
}  // e131
}  // plugin
}  // ola
