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
 * OSCDevice.cpp
 * The OSC Device.
 * Copyright (C) 2012 Simon Newton
 */

#include <sstream>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/io/SelectServerInterface.h"
#include "plugins/osc/OSCDevice.h"
#include "plugins/osc/OSCPort.h"

namespace ola {
namespace plugin {
namespace osc {

using std::ostringstream;
using std::vector;

const char OSCDevice::DEVICE_NAME[] = "OSC Device";

/**
 * Constructor for the OSCDevice
 * @param owner the plugin which created this device
 * @param plugin_adaptor a pointer to a PluginAdaptor object
 * @param udp_port the UDP port to listen on
 * @param addresses a list of strings to use as OSC addresses for the input
 *   ports.
 * @param targets a vector-of-vectors of OSCTargets to use for the output
 *   ports.
 */
OSCDevice::OSCDevice(AbstractPlugin *owner,
                     PluginAdaptor *plugin_adaptor,
                     uint16_t udp_port,
                     const vector<string> &addresses,
                     const vector<vector<OSCTarget> > &targets)
    : Device(owner, DEVICE_NAME),
      m_plugin_adaptor(plugin_adaptor),
      m_port_addresses(addresses),
      m_port_targets(targets) {
  OSCNode::OSCNodeOptions options;
  options.listen_port = udp_port;
  // allocate a new OSCNode but delay the call to Init() until later
  m_osc_node.reset(new OSCNode(plugin_adaptor, plugin_adaptor->GetExportMap(),
                               options));
}

/*
 * Start this device.
 * @returns true if the device started successfully, false otherwise.
 */
bool OSCDevice::StartHook() {
  bool ok = true;
  if (!m_osc_node->Init())
    return false;

  // Create an input port for each OSC Address.
  for (unsigned int i = 0; i < m_port_addresses.size(); ++i) {
    OSCInputPort *port = new OSCInputPort(this, i, m_plugin_adaptor,
                                          m_osc_node.get(),
                                          m_port_addresses[i]);
    if (!AddPort(port)) {
      delete port;
      ok = false;
    }
  }

  // Create an output port for each list of OSC Targets.
  for (unsigned int i = 0; i < m_port_targets.size(); ++i) {
    const vector<OSCTarget> &targets = m_port_targets[i];
    ostringstream str;

    if (targets.empty()) {
      OLA_INFO << "No targets specified for OSC Output port " << i;
      continue;
    }

    vector<OSCTarget>::const_iterator iter = targets.begin();
    for (; iter != targets.end(); ++iter) {
      if (iter != targets.begin())
        str << ", ";
      str << iter->socket_address << iter->osc_address;
      m_osc_node->AddTarget(i, *iter);
    }
    OSCOutputPort *port = new OSCOutputPort(this, i, m_osc_node.get(),
                                            str.str());
    if (!AddPort(port)) {
      delete port;
      ok = false;
    }
  }
  return ok;
}
}  // osc
}  // plugin
}  // ola
