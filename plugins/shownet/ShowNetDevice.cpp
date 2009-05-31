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
 * ShowNetDevice.cpp
 * ShowNet device
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <lla/Logging.h>
#include <llad/Plugin.h>
#include <llad/PluginAdaptor.h>
#include <llad/Preferences.h>

#include "ShowNetDevice.h"
#include "ShowNetPort.h"
#include "ShowNetNode.h"

namespace lla {
namespace shownet {

const std::string ShowNetDevice::IP_KEY = "ip";


/*
 * Create a new device
 */
ShowNetDevice::ShowNetDevice(lla::Plugin *owner,
                             const string &name,
                             Preferences *preferences,
                             const PluginAdaptor *plugin_adaptor):
  Device(owner, name),
  m_preferences(preferences),
  m_plugin_adaptor(plugin_adaptor),
  m_node(NULL),
  m_enabled(false) {}


/*
 * Start this device
 */
bool ShowNetDevice::Start() {
  if (m_enabled)
    return false;

  ShowNetPort *port = NULL;

  for (unsigned int i = 0; i < 2 * ShowNetNode::SHOWNET_MAX_UNIVERSES; i++) {
    port = new ShowNetPort(this, i);
    this->AddPort(port);
  }

  m_node = new ShowNetNode(m_preferences->GetValue("ip"));
  m_node->SetName(m_preferences->GetValue("name"));

  if (!m_node->Start()) {
    delete m_node;
    m_node = NULL;
    DeleteAllPorts();
    return false;
  }

  m_plugin_adaptor->AddSocket(m_node->GetSocket());
  m_enabled = true;
  return true;
}


/*
 * Stop this device
 */
bool ShowNetDevice::Stop() {
  if (!m_enabled)
    return false;

  DeleteAllPorts();
  m_node->Stop();
  delete m_node;
  m_node = NULL;
  m_enabled = false;
  return true;
}


} //plugin
} //lla
