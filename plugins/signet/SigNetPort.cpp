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
 * SigNetPort.cpp
 * The SigNetInputPort and SigNetOutputPort classes.
 * Copyright (C) 2026 Peter Newman
 */

#include <sstream>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "olad/Port.h"
#include "plugins/signet/SigNetDevice.h"
#include "plugins/signet/SigNetEnums.h"
#include "plugins/signet/SigNetNode.h"
#include "plugins/signet/SigNetPort.h"

namespace ola {
namespace plugin {
namespace signet {

using std::ostringstream;
using std::string;
using std::vector;

bool SigNetPortHelper::PreSetUniverse(Universe *old_universe,
                                      Universe *new_universe) {
  if (new_universe && ((new_universe->UniverseId() < SIGNET_MIN_UNIVERSE) ||
                       (new_universe->UniverseId() > SIGNET_MAX_UNIVERSE))) {
    OLA_WARN << "Universe id " << new_universe->UniverseId()
             << " is < " << SIGNET_MIN_UNIVERSE
             << " or > " << SIGNET_MAX_UNIVERSE;
    return false;
  }
  (void) old_universe;
  return true;
}


string SigNetPortHelper::Description(Universe *universe) const {
  std::ostringstream str;
  if (universe)
    str << "Sig-Net Universe " << universe->UniverseId();
  return str.str();
}


SigNetInputPort::SigNetInputPort(
  SigNetDevice *parent,
  unsigned int port_id,
  PluginAdaptor *plugin_adaptor,
  SigNetNode *node)
  : BasicInputPort(parent, port_id, plugin_adaptor),
    m_node(node),
    m_priority(ola::dmx::SOURCE_PRIORITY_DEFAULT) {
  SetPriorityMode(PRIORITY_MODE_INHERIT);
}


/*
 * Set the universe for an input port.
 */
void SigNetInputPort::PostSetUniverse(Universe *old_universe,
                                      Universe *new_universe) {
  if (old_universe) {
    m_node->RemoveHandler(old_universe->UniverseId());
  }

  if (new_universe) {
    // TODO(Peter): What callback do we actually need here?
    m_node->SetHandler(
        new_universe->UniverseId(),
        &m_buffer,
        &m_priority,
        NewCallback(this, &SigNetInputPort::NewDMXData));
  }
}

void SigNetInputPort::NewDMXData(const DmxBuffer &data) {
  m_buffer = data;  // store the data
  DmxChanged();  // signal that our data has changed
}


SigNetOutputPort::SigNetOutputPort(
  SigNetDevice *device,
  unsigned int port_id,
  SigNetNode *node)
  : BasicOutputPort(device, port_id),
    m_node(node) {
}

/*
 * Set the universe for an output port.
 */
void SigNetOutputPort::PostSetUniverse(Universe *old_universe,
                                       Universe *new_universe) {
  if (old_universe) {
//    m_node->TerminateStream(old_universe->UniverseId(), m_last_priority);
  }
  if (new_universe) {
//    m_node->StartStream(new_universe->UniverseId());
  }
}

SigNetOutputPort::~SigNetOutputPort() {
}
}  // namespace signet
}  // namespace plugin
}  // namespace ola
