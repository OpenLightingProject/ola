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
 * OscDevice.h
 * The OSC Device.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef PLUGINS_OSC_OSCDEVICE_H_
#define PLUGINS_OSC_OSCDEVICE_H_

#include <memory>
#include <string>
#include <vector>
#include "ola/io/SelectServerInterface.h"
#include "ola/network/SocketAddress.h"
#include "olad/Device.h"
#include "plugins/osc/OscTarget.h"
#include "plugins/osc/OscNode.h"

namespace ola {

class AbstractPlugin;

namespace plugin {
namespace osc {

class OscDevice: public Device {
 public:
    struct PortConfig {
      PortConfig() : data_format(OscNode::FORMAT_BLOB) {}

      std::vector<OscTarget> targets;
      OscNode::DataFormat data_format;
    };

    typedef std::vector<PortConfig> PortConfigs;

    OscDevice(AbstractPlugin *owner,
              PluginAdaptor *plugin_adaptor,
              uint16_t udp_port,
              const std::vector<std::string> &addresses,
              const PortConfigs &port_configs);
    std::string DeviceId() const { return "1"; }

    bool AllowLooping() const { return true; }
    bool AllowMultiPortPatching() const { return true; }

 protected:
    PluginAdaptor *m_plugin_adaptor;
    const std::vector<std::string> m_port_addresses;
    const PortConfigs m_port_configs;
    std::auto_ptr<class OscNode> m_osc_node;

    bool StartHook();

    static const char DEVICE_NAME[];
};
}  // namespace osc
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OSC_OSCDEVICE_H_
