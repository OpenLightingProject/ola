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
 * Dmx4LinuxPlugin.h
 * Interface for the dmx4linux plugin class
 * Copyright (C) 2006-2009 Simon Newton
 */

#ifndef DMX4LINUXPLUGIN_H
#define DMX4LINUXPLUGIN_H

#include <vector>
#include <string>
#include <lla/DmxBuffer.h>
#include <llad/Plugin.h>
#include <lla/network/Socket.h>
#include <lla/plugin_id.h>

namespace lla {
namespace plugin {

class Dmx4LinuxDevice;
using lla::network::SocketListener;
using lla::network::ConnectedSocket;

class Dmx4LinuxPlugin: public lla::Plugin, public SocketListener {
  public:
    Dmx4LinuxPlugin(const PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_in_socket(NULL),
      m_out_socket(NULL) {}
    ~Dmx4LinuxPlugin();

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    lla_plugin_id Id() const { return LLA_PLUGIN_DMX4LINUX; }

    int SocketReady(lla::network::ConnectedSocket *socket);
    bool SendDMX(int d4l_uni, const DmxBuffer &buffer) const;

  protected:
    string PreferencesSuffix() const { return PLUGIN_PREFIX; }

  private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    bool SetupSockets();
    int CleanupSockets();
    int GetDmx4LinuxDeviceCount(int dir);
    bool SetupDevice(string family, int d4l_uni, int dir);
    bool SetupDevices(int dir);
    bool Setup();

    vector<Dmx4LinuxDevice*>  m_devices;  // list of out devices
    string m_out_dev;  // path the the dmx device
    string m_in_dev;   // path the the dmx device
    ConnectedSocket *m_in_socket;
    ConnectedSocket *m_out_socket;

    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
};

} //plugin
} //lla

#endif
