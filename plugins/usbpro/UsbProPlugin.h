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
 * UsbProPlugin.h
 * Interface for the usbpro plugin class
 * Copyright (C) 2006  Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBPROPLUGIN_H_
#define PLUGINS_USBPRO_USBPROPLUGIN_H_

#include <string>
#include <vector>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"
#include "ola/network/Socket.h"

namespace ola {
namespace usbpro {

using ola::network::ConnectedSocket;

class UsbProDevice;

class UsbProPlugin: public ola::Plugin {
  public:
    explicit UsbProPlugin(const PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor) {}

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_USBPRO; }
    int SocketClosed(ConnectedSocket *socket);
    string PluginPrefix() const { return PLUGIN_PREFIX; }

  private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();
    void DeleteDevice(UsbProDevice *device);
    vector<string> FindCandiateDevices();

    vector<UsbProDevice*> m_devices;  // list of our devices

    static const char USBPRO_DEVICE_NAME[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char DEVICE_DIR_KEY[];
    static const char DEVICE_PREFIX_KEY[];
    static const char DEFAULT_DEVICE_DIR[];
    static const char LINUX_DEVICE_PREFIX[];
    static const char MAC_DEVICE_PREFIX[];
};
}  // usbpro
}  // ola
#endif  // PLUGINS_USBPRO_USBPROPLUGIN_H_
