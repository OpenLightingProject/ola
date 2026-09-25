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
 * SigNetDevice.cpp
 * The SigNet Device.
 * Copyright (C) 2026 Peter Newman
 */

#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/io/SelectServerInterface.h"
#include "plugins/signet/SigNetDevice.h"
#include "plugins/signet/SigNetPort.h"

namespace ola {
namespace plugin {
namespace signet {

using std::string;
using std::vector;

const char SigNetDevice::DEVICE_NAME[] = "SigNet Device";

/**
 * Constructor for the SigNetDevice
 * @param owner the plugin which created this device
 * @param plugin_adaptor a pointer to a PluginAdaptor object
 * @param options the options to use for the device
 */
SigNetDevice::SigNetDevice(AbstractPlugin *owner,
                           PluginAdaptor *plugin_adaptor,
                           const SigNetDeviceOptions &options)
    : Device(owner, DEVICE_NAME),
      m_plugin_adaptor(plugin_adaptor),
      m_options(options) {
  SigNetNode::SigNetNodeOptions node_options;
  // allocate a new SigNetNode but delay the call to Init() until later
  m_signet_node.reset(new SigNetNode(plugin_adaptor,
                                     plugin_adaptor->GetExportMap(),
                                     plugin_adaptor->DefaultUID(),
                                     node_options));
}

/*
 * Start this device.
 * @returns true if the device started successfully, false otherwise.
 */
bool SigNetDevice::StartHook() {
  bool ok = true;
  if (!m_signet_node->Init()) {
    OLA_FATAL << "SigNet Node didn't init!";
    return false;
  }

  for (unsigned int i = 0; i < m_options.input_ports; i++) {
    SigNetInputPort *input_port = new SigNetInputPort(
        this, i, m_plugin_adaptor, m_signet_node.get());
    if (!AddPort(input_port)) {
      delete input_port;
      ok = false;
    }
    m_input_ports.push_back(input_port);
  }

  for (unsigned int i = 0; i < m_options.output_ports; i++) {
    SigNetOutputPort *output_port = new SigNetOutputPort(
        this, i, m_signet_node.get());
    if (!AddPort(output_port)) {
      delete output_port;
      ok = false;
    }
    m_output_ports.push_back(output_port);
  }

  return ok;
}

SigNetInputPort *SigNetDevice::GetSigNetInputPort(unsigned int port_id) {
  return (port_id < m_input_ports.size()) ? m_input_ports[port_id] : NULL;
}

SigNetOutputPort *SigNetDevice::GetSigNetOutputPort(unsigned int port_id) {
  return (port_id < m_output_ports.size()) ? m_output_ports[port_id] : NULL;
}
}  // namespace signet
}  // namespace plugin
}  // namespace ola
