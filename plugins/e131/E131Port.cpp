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
 * E131Port.cpp
 * The E1.31 plugin for ola
 * Copyright (C) 2007 Simon Newton
 */

#include <string>
#include "ola/Logging.h"
#include "olad/Universe.h"
#include "plugins/e131/E131Port.h"
#include "plugins/e131/E131Device.h"

namespace ola {
namespace plugin {
namespace e131 {

using std::string;

bool E131PortHelper::PreSetUniverse(Universe *old_universe,
                                    Universe *new_universe) {
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
  std::ostringstream str;
  if (universe)
    str << "E1.31 Universe " << universe->UniverseId();
  return str.str();
}



/*
 * Set the universe for an input port.
 */
void E131InputPort::PostSetUniverse(Universe *old_universe,
                                    Universe *new_universe) {
  if (old_universe)
    m_node->RemoveHandler(old_universe->UniverseId());

  if (new_universe)
    m_node->SetHandler(
        new_universe->UniverseId(),
        &m_buffer,
        &m_priority,
        NewCallback<E131InputPort, void>(this, &E131InputPort::DmxChanged));
}

E131OutputPort::~E131OutputPort() {
  Universe *universe = GetUniverse();
  if (universe) {
    m_node->TerminateStream(universe->UniverseId(), m_last_priority);
  }
}

/*
 * Set the universe for an output port.
 */
void E131OutputPort::PostSetUniverse(Universe *old_universe,
                                     Universe *new_universe) {
  if (old_universe) {
    m_node->TerminateStream(old_universe->UniverseId(), m_last_priority);
  }
  if (new_universe) {
    m_node->StartStream(new_universe->UniverseId());
  }
}


/*
 * Write data to this port.
 */
bool E131OutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
  Universe *universe = GetUniverse();
  if (!universe)
    return false;

  m_last_priority = (GetPriorityMode() == PRIORITY_MODE_STATIC) ?
      GetPriority() : priority;
  return m_node->SendDMX(universe->UniverseId(), buffer, m_last_priority,
                         m_preview_on);
}
}  // namespace e131
}  // namespace plugin
}  // namespace ola
