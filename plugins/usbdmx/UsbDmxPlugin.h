/*
 *  This dmxgram is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This dmxgram is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this dmxgram; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * UsbDmxPlugin.h
 * Interface for the usbdmx plugin class
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBDMX_USBDMXPLUGIN_H_
#define PLUGINS_USBDMX_USBDMXPLUGIN_H_

#include <libusb.h>
#include <string>
#include <vector>
#include "ola/plugin_id.h"
#include "olad/Plugin.h"
#include "ola/network/Socket.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::network::ConnectedSocket;

class UsbDmxDevice;

class UsbDmxPlugin: public ola::Plugin {
  public:
    explicit UsbDmxPlugin(const PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_usb_context(NULL) {}

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_USBDMX; }
    string PluginPrefix() const { return PLUGIN_PREFIX; }

    bool AddDeviceSocket(int fd);
    bool RemoveDeviceSocket(int fd);
    int SocketReady();

  private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();
    vector<UsbDmxDevice*> m_devices;  // list of our devices
    libusb_context *m_usb_context;
    vector<ola::network::DeviceSocket*> m_sockets;

    static const char USBDMX_DEVICE_NAME[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
};
}  // usbdmx
}  // plugin
}  // ola
#endif  // PLUGINS_USBDMX_USBDMXPLUGIN_H_
