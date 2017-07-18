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
 * OscDevice.cpp
 * The OSC Device.
 * Copyright (C) 2012 Simon Newton
 */

#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/io/SelectServerInterface.h"
#include "plugins/osc/OscDevice.h"
#include "plugins/osc/OscPort.h"

namespace ola {
namespace plugin {
namespace osc {

using std::string;
using std::vector;

const char OscDevice::DEVICE_NAME[] = "OSC Device";

/**
 * Constructor for the OscDevice
 * @param owner the plugin which created this device
 * @param plugin_adaptor a pointer to a PluginAdaptor object
 * @param udp_port the UDP port to listen on
 * @param addresses a list of strings to use as OSC addresses for the input
 *   ports.
 * @param port_configs config to use for the ports
 */
OscDevice::OscDevice(AbstractPlugin *owner,
                     PluginAdaptor *plugin_adaptor,
                     uint16_t udp_port,
                     const vector<string> &addresses,
                     const PortConfigs &port_configs)
    : Device(owner, DEVICE_NAME),
      m_plugin_adaptor(plugin_adaptor),
      m_port_addresses(addresses),
      m_port_configs(port_configs) {
  OscNode::OscNodeOptions options;
  options.listen_port = udp_port;
  // allocate a new OscNode but delay the call to Init() until later
  m_osc_node.reset(new OscNode(plugin_adaptor, plugin_adaptor->GetExportMap(),
                               options));
}

/*
 * Start this device.
 * @returns true if the device started successfully, false otherwise.
 */
bool OscDevice::StartHook() {
  bool ok = true;
  if (!m_osc_node->Init())
    return false;

  // Create an input port for each OSC Address.
  for (unsigned int i = 0; i < m_port_addresses.size(); ++i) {
    OscInputPort *port = new OscInputPort(this, i, m_plugin_adaptor,
                                          m_osc_node.get(),
                                          m_port_addresses[i]);
    if (!AddPort(port)) {
      delete port;
      ok = false;
    }
  }

  // Create an output port for each list of OSC Targets.
  PortConfigs::const_iterator port_iter = m_port_configs.begin();
  for (int i = 0; port_iter != m_port_configs.end(); ++port_iter, ++i) {
    const PortConfig &port_config = *port_iter;
    if (port_config.targets.empty()) {
      OLA_INFO << "No targets specified for OSC Output port " << i;
      continue;
    }

    OscOutputPort *port = new OscOutputPort(this, i, m_osc_node.get(),
                                            port_config.targets,
                                            port_config.data_format);
    if (!AddPort(port)) {
      delete port;
      ok = false;
    }
  }
  return ok;
}
}  // namespace osc
}  // namespace plugin
}  // namespace ola
