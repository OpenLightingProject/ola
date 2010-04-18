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
#include "plugins/usbpro/WidgetDetector.h"
#include "plugins/usbpro/UsbDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::network::ConnectedSocket;

class UsbProPlugin: public ola::Plugin, WidgetDetectorListener {
  public:
    explicit UsbProPlugin(const PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_detector(plugin_adaptor) {}

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_USBPRO; }
    int DeviceRemoved(UsbDevice *device);
    string PluginPrefix() const { return PLUGIN_PREFIX; }

    void NewWidget(class UsbWidget *widget,
                   const DeviceInformation &information);
    void AddDevice(UsbDevice *device);

  private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();
    void DeleteDevice(UsbDevice *device);
    vector<string> FindCandiateDevices();

    vector<UsbDevice*> m_devices;  // list of our devices
    WidgetDetector m_detector;

    static const char USBPRO_DEVICE_NAME[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char DEVICE_DIR_KEY[];
    static const char DEVICE_PREFIX_KEY[];
    static const char DEFAULT_DEVICE_DIR[];
    static const char LINUX_DEVICE_PREFIX[];
    static const char MAC_DEVICE_PREFIX[];

    static const uint16_t OPEN_LIGHTING_ESTA_ID = 0x7a70;
    static const uint16_t JESE_ESTA_ID = 0x6864;
    static const uint16_t DMX_KING_ESTA_ID = 0x6a6b;
    static const uint16_t OPEN_LIGHTING_RGB_MIXER_ID = 1;
    static const uint16_t JESE_DMX_TRI_ID = 1;
    static const uint16_t DMX_KING_DEVICE_ID = 0;
    static const uint16_t ENTTEC_ESTA_ID = 0x454E;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_USBPROPLUGIN_H_
