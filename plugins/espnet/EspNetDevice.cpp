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
 * EspNetDevice.cpp
 * Esp-Net device
 * Copyright (C) 2005 Simon Newton
 *
 *
 */

#include <memory>
#include <sstream>
#include <string>
#include "ola/Logging.h"
#include "ola/network/Interface.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"

#include "plugins/espnet/EspNetDevice.h"
#include "plugins/espnet/EspNetNode.h"
#include "plugins/espnet/EspNetPluginCommon.h"
#include "plugins/espnet/EspNetPort.h"

namespace ola {
namespace plugin {
namespace espnet {

using std::ostringstream;
using std::string;

const char EspNetDevice::ESPNET_DEVICE_NAME[] = "ESP Net";
const char EspNetDevice::IP_KEY[] = "ip";
const char EspNetDevice::NODE_NAME_KEY[] = "name";

/*
 * Create a new device
 */
EspNetDevice::EspNetDevice(Plugin *owner,
                           Preferences *prefs,
                           PluginAdaptor *plugin_adaptor):
  Device(owner, ESPNET_DEVICE_NAME),
  m_preferences(prefs),
  m_plugin_adaptor(plugin_adaptor),
  m_node(NULL) {
}


/*
 * Start this device
 */
bool EspNetDevice::StartHook() {
  string ip_address = m_preferences->GetValue(IP_KEY);
  ola::network::Interface interface;
  std::auto_ptr<ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
  if (!picker->ChooseInterface(&interface, ip_address,
                               m_plugin_adaptor->DefaultInterface())) {
    OLA_INFO << "Failed to find an interface";
    return false;
  }
  m_node = new EspNetNode(interface);
  m_node->SetName(m_preferences->GetValue(NODE_NAME_KEY));
  m_node->SetType(ESPNET_NODE_TYPE_IO);

  if (!m_node->Start()) {
    delete m_node;
    m_node = NULL;
    return false;
  }

  ostringstream str;
  str << ESPNET_DEVICE_NAME << " [" << m_node->GetInterface().ip_address <<
    "]";
  SetName(str.str());

  for (unsigned int i = 0; i < PORTS_PER_DEVICE; i++) {
    EspNetInputPort *input_port = new EspNetInputPort(
        this,
        i,
        m_plugin_adaptor,
        m_node);
    AddPort(input_port);
    EspNetOutputPort *output_port = new EspNetOutputPort(this, i, m_node);
    AddPort(output_port);
  }

  m_plugin_adaptor->AddReadDescriptor(m_node->GetSocket());
  return true;
}


/*
 * Stop this device
 */
void EspNetDevice::PrePortStop() {
  m_plugin_adaptor->RemoveReadDescriptor(m_node->GetSocket());
}

/*
 * Stop this device
 */
void EspNetDevice::PostPortStop() {
  m_node->Stop();
  delete m_node;
  m_node = NULL;
}
}  // namespace espnet
}  // namespace plugin
}  // namespace ola
