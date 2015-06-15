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
 * ShowNetDevice.cpp
 * ShowNet device
 * Copyright (C) 2005 Simon Newton
 */

#include <sstream>
#include <string>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/shownet/ShowNetDevice.h"
#include "plugins/shownet/ShowNetNode.h"
#include "plugins/shownet/ShowNetPort.h"

namespace ola {
namespace plugin {
namespace shownet {

using std::ostringstream;

const char ShowNetDevice::SHOWNET_DEVICE_NAME[] = "ShowNet";
const char ShowNetDevice::IP_KEY[] = "ip";


/*
 * Create a new device
 */
ShowNetDevice::ShowNetDevice(ola::Plugin *owner,
                             Preferences *preferences,
                             PluginAdaptor *plugin_adaptor):
  Device(owner, SHOWNET_DEVICE_NAME),
  m_preferences(preferences),
  m_plugin_adaptor(plugin_adaptor),
  m_node(NULL) {}


/*
 * Start this device
 */
bool ShowNetDevice::StartHook() {
  m_node = new ShowNetNode(m_preferences->GetValue(IP_KEY));
  m_node->SetName(m_preferences->GetValue("name"));

  if (!m_node->Start()) {
    delete m_node;
    m_node = NULL;
    DeleteAllPorts();
    return false;
  }

  ostringstream str;
  str << SHOWNET_DEVICE_NAME << " [" << m_node->GetInterface().ip_address <<
    "]";
  SetName(str.str());


  for (unsigned int i = 0; i < ShowNetNode::SHOWNET_MAX_UNIVERSES; i++) {
    ShowNetInputPort *input_port = new ShowNetInputPort(
        this,
        i,
        m_plugin_adaptor,
        m_node);
    AddPort(input_port);
    ShowNetOutputPort *output_port = new ShowNetOutputPort(this, i, m_node);
    AddPort(output_port);
  }

  m_plugin_adaptor->AddReadDescriptor(m_node->GetSocket());
  return true;
}


/*
 * Stop this device
 */
void ShowNetDevice::PrePortStop() {
  m_plugin_adaptor->RemoveReadDescriptor(m_node->GetSocket());
}


/*
 * Stop this device
 */
void ShowNetDevice::PostPortStop() {
  m_node->Stop();
  delete m_node;
  m_node = NULL;
}
}  // namespace shownet
}  // namespace plugin
}  // namespace ola
