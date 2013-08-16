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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * OSCPort.coo
 * The OSCInputPort and OSCOutputPort classes.
 * Copyright (C) 2012 Simon Newton
 */

#include <sstream>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "olad/Port.h"
#include "plugins/osc/OSCAddressTemplate.h"
#include "plugins/osc/OSCDevice.h"
#include "plugins/osc/OSCNode.h"
#include "plugins/osc/OSCPort.h"

namespace ola {
namespace plugin {
namespace osc {

using std::ostringstream;

OSCInputPort::OSCInputPort(
    OSCDevice *parent,
    unsigned int port_id,
    PluginAdaptor *plugin_adaptor,
    OSCNode *node,
    const string &address)
    : BasicInputPort(parent, port_id, plugin_adaptor),
      m_node(node),
      m_address(address),
      m_actual_address(address) {
}

bool OSCInputPort::PreSetUniverse(Universe *old_universe,
                                  Universe *new_universe) {
  // if the old_universe is not NULL, we need to de-register
  if (old_universe) {
    m_node->RegisterAddress(m_actual_address, NULL);
    // reset the actual address
    m_actual_address = m_address;
  }

  // if we've been supplied with a new universe, attempt to register
  if (new_universe) {
    string osc_address = ExpandTemplate(m_address, new_universe->UniverseId());
    bool ok = m_node->RegisterAddress(
        osc_address,
        NewCallback(this, &OSCInputPort::NewDMXData));

    if (!ok)
      // means that another port is registered with the same address
      return false;
    // update the address since the registration was successful.
    m_actual_address = osc_address;
  }
  return true;
}

void OSCInputPort::NewDMXData(const DmxBuffer &data) {
  m_buffer = data;  // store the data
  DmxChanged();  // signal that our data has changed
}


OSCOutputPort::OSCOutputPort(
    OSCDevice *device,
    unsigned int port_id,
    OSCNode *node,
    const vector<OSCTarget> &targets,
    OSCNode::DataFormat data_format)
    : BasicOutputPort(device, port_id),
      m_node(node),
      m_template_targets(targets),
      m_data_format(data_format) {
  SetUnpatchedDescription();
}

OSCOutputPort::~OSCOutputPort() {
  RemoveTargets();
}

bool OSCOutputPort::PreSetUniverse(Universe *,
                                   Universe *new_universe) {
  RemoveTargets();

  vector<OSCTarget>::const_iterator iter = m_template_targets.begin();

  if (new_universe) {
    ostringstream str;
    for (; iter != m_template_targets.end(); ++iter) {
      string osc_address = ExpandTemplate(iter->osc_address,
                                          new_universe->UniverseId());
      OSCTarget new_target(iter->socket_address, osc_address);

      m_node->AddTarget(PortId(), new_target);
      m_registered_targets.push_back(new_target);

      if (iter != m_template_targets.begin())
        str << ", ";
      str << new_target;
    }
    m_description = str.str();
  } else {
    SetUnpatchedDescription();
  }

  return true;
}

void OSCOutputPort::RemoveTargets() {
  vector<OSCTarget>::const_iterator iter = m_registered_targets.begin();
  for (; iter != m_registered_targets.end(); ++iter) {
    m_node->RemoveTarget(PortId(), *iter);
  }
  m_registered_targets.clear();
}

void OSCOutputPort::SetUnpatchedDescription() {
  ostringstream str;
  vector<OSCTarget>::const_iterator iter = m_template_targets.begin();
  for (; iter != m_template_targets.end(); ++iter) {
    if (iter != m_template_targets.begin())
      str << ", ";
    str << *iter;
  }
  m_description = str.str();
}
}  // namespace osc
}  // namespace plugin
}  // namespace ola
