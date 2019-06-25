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
 * UsbSerialPlugin.h
 * Interface for the usbpro plugin class
 * Copyright (C) 2006 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBSERIALPLUGIN_H_
#define PLUGINS_USBPRO_USBSERIALPLUGIN_H_

#include <string>
#include <vector>
#include "ola/io/Descriptor.h"
#include "ola/plugin_id.h"
#include "olad/Plugin.h"
#include "plugins/usbpro/UsbSerialDevice.h"
#include "plugins/usbpro/WidgetDetectorThread.h"

namespace ola {
namespace plugin {
namespace usbpro {

class UsbSerialPlugin: public ola::Plugin, public NewWidgetHandler {
 public:
    explicit UsbSerialPlugin(PluginAdaptor *plugin_adaptor);

    std::string Name() const { return PLUGIN_NAME; }
    std::string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_USBPRO; }
    void DeviceRemoved(UsbSerialDevice *device);
    std::string PluginPrefix() const { return PLUGIN_PREFIX; }

    void NewWidget(ArduinoWidget *widget,
                   const UsbProWidgetInformation &information);
    void NewWidget(EnttecUsbProWidget *widget,
                   const UsbProWidgetInformation &information);
    void NewWidget(DmxTriWidget *widget,
                   const UsbProWidgetInformation &information);
    void NewWidget(DmxterWidget *widget,
                   const UsbProWidgetInformation &information);
    void NewWidget(DMXUSBWidget *widget,
                   const UsbProWidgetInformation &information);
    void NewWidget(RobeWidget *widget,
                   const RobeWidgetInformation &information);
    void NewWidget(UltraDMXProWidget *widget,
                   const UsbProWidgetInformation &information);

 private:
    void AddDevice(UsbSerialDevice *device);
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();
    void DeleteDevice(UsbSerialDevice *device);
    std::string GetDeviceName(const UsbProWidgetInformation &information);
    unsigned int GetProFrameLimit();
    unsigned int GetDmxTriFrameLimit();
    unsigned int GetUltraDMXProFrameLimit();

    std::vector<UsbSerialDevice*> m_devices;  // list of our devices
    WidgetDetectorThread m_detector_thread;

    static const char DEFAULT_DEVICE_DIR[];
    static const char DEVICE_DIR_KEY[];
    static const char DEVICE_PREFIX_KEY[];
    static const char IGNORED_DEVICES_KEY[];
    static const char LINUX_DEVICE_PREFIX[];
    static const char BSD_DEVICE_PREFIX[];
    static const char MAC_DEVICE_PREFIX[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char ROBE_DEVICE_NAME[];
    static const char TRI_USE_RAW_RDM_KEY[];
    static const char USBPRO_DEVICE_NAME[];
    static const char USB_PRO_FPS_LIMIT_KEY[];
    static const char ULTRA_FPS_LIMIT_KEY[];

    static const uint8_t DEFAULT_PRO_FPS_LIMIT = 190;
    static const uint8_t DEFAULT_ULTRA_FPS_LIMIT = 40;
    static const unsigned int MAX_PRO_FPS_LIMIT = 1000;
    static const unsigned int MAX_ULTRA_FPS_LIMIT = 1000;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_USBSERIALPLUGIN_H_
