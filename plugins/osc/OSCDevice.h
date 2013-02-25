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
 * OSCDevice.h
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
#include "plugins/osc/OSCNode.h"
#include "plugins/osc/OSCTarget.h"

namespace ola {

class AbstractPlugin;

namespace plugin {
namespace osc {

using std::string;
using ola::network::IPV4SocketAddress;

class OSCDevice: public Device {
  public:
    OSCDevice(AbstractPlugin *owner,
              PluginAdaptor *plugin_adaptor,
              uint16_t udp_port,
              const vector<string> &addresses,
              const vector<vector<OSCTarget> > &targets);
    string DeviceId() const { return "1"; }

    bool AllowMultiPortPatching() const { return true; }

  protected:
    PluginAdaptor *m_plugin_adaptor;
    const vector<string> m_port_addresses;
    const vector<vector<OSCTarget> > m_port_targets;
    std::auto_ptr<OSCNode> m_osc_node;

    bool StartHook();

    static const char DEVICE_NAME[];
};
}  // osc
}  // plugin
}  // ola
#endif  // PLUGINS_OSC_OSCDEVICE_H_
