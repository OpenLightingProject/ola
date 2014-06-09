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
 * Dmx4LinuxPlugin.h
 * Interface for the dmx4linux plugin class
 * Copyright (C) 2006 Simon Newton
 */

#ifndef PLUGINS_DMX4LINUX_DMX4LINUXPLUGIN_H_
#define PLUGINS_DMX4LINUX_DMX4LINUXPLUGIN_H_

#include <vector>
#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Plugin.h"
#include "ola/network/Socket.h"
#include "ola/plugin_id.h"
#include "plugins/dmx4linux/Dmx4LinuxPort.h"
#include "plugins/dmx4linux/Dmx4LinuxSocket.h"

namespace ola {
namespace plugin {
namespace dmx4linux {

class Dmx4LinuxDevice;

class Dmx4LinuxPlugin: public ola::Plugin {
 public:
    explicit Dmx4LinuxPlugin(PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_in_descriptor(NULL),
      m_out_descriptor(NULL),
      m_in_devices_count(0),
      m_in_buffer(NULL) {}
    ~Dmx4LinuxPlugin();

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_DMX4LINUX; }

    int SocketReady();
    string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    bool SetupDescriptors();
    int CleanupDescriptors();
    bool SetupDevice(string family, int d4l_uni, int dir);
    bool SetupDevices();

    vector<Dmx4LinuxDevice*>  m_devices;  // list of out devices
    vector<Dmx4LinuxInputPort*>  m_in_ports;  // list of in ports
    string m_out_dev;  // path to the dmx output device
    string m_in_dev;   // path to the dmx input device
    Dmx4LinuxSocket *m_in_descriptor;
    Dmx4LinuxSocket *m_out_descriptor;
    int m_in_devices_count;  // number of input devices
    uint8_t *m_in_buffer;  // input buffer

    static const char DMX4LINUX_OUT_DEVICE[];
    static const char DMX4LINUX_IN_DEVICE[];
    static const char OUT_DEV_KEY[];
    static const char IN_DEV_KEY[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
};
}  // namespace dmx4linux
}  // namespace plugin
}  // namespace ola

#endif  // PLUGINS_DMX4LINUX_DMX4LINUXPLUGIN_H_
