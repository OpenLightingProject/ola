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
 * UsbSerialPlugin.h
 * Interface for the usbpro plugin class
 * Copyright (C) 2006  Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBPROPLUGIN_H_
#define PLUGINS_USBPRO_USBPROPLUGIN_H_

#include <string>
#include <vector>
#include "ola/network/Socket.h"
#include "ola/plugin_id.h"
#include "olad/Plugin.h"
#include "plugins/usbpro/UsbSerialDevice.h"
#include "plugins/usbpro/WidgetDetectorThread.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::network::ConnectedDescriptor;

class UsbSerialPlugin: public ola::Plugin, public NewWidgetHandler {
  public:
    explicit UsbSerialPlugin(PluginAdaptor *plugin_adaptor);

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_USBPRO; }
    void DeviceRemoved(UsbSerialDevice *device);
    string PluginPrefix() const { return PLUGIN_PREFIX; }

    void NewWidget(ArduinoWidget *widget,
                   const UsbProWidgetInformation &information);
    void NewWidget(EnttecUsbProWidget *widget,
                   const UsbProWidgetInformation &information);
    void NewWidget(DmxTriWidget *widget,
                   const UsbProWidgetInformation &information);
    void NewWidget(DmxterWidget *widget,
                   const UsbProWidgetInformation &information);
    void NewWidget(RobeWidget *widget,
                   const RobeWidgetInformation &information);

  private:
    void AddDevice(UsbSerialDevice *device);
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();
    void DeleteDevice(UsbSerialDevice *device);
    string GetDeviceName(const UsbProWidgetInformation &information);
    unsigned int GetProFrameLimit();

    vector<UsbSerialDevice*> m_devices;  // list of our devices
    WidgetDetectorThread m_detector_thread;

    static const char DEFAULT_DEVICE_DIR[];
    static const char DEFAULT_PRO_FPS_LIMIT[];
    static const char DEVICE_DIR_KEY[];
    static const char DEVICE_PREFIX_KEY[];
    static const char LINUX_DEVICE_PREFIX[];
    static const char MAC_DEVICE_PREFIX[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char ROBE_DEVICE_NAME[];
    static const char TRI_USE_RAW_RDM_KEY[];
    static const char USBPRO_DEVICE_NAME[];
    static const char USB_PRO_FPS_LIMIT_KEY[];
    static const unsigned int MAX_PRO_FPS_LIMIT = 1000;

    static const uint16_t ENTTEC_ESTA_ID = 0x454E;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_USBPROPLUGIN_H_
