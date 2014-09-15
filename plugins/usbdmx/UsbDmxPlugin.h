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
 * UsbDmxPlugin.h
 * Interface for the usbdmx plugin class
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBDMX_USBDMXPLUGIN_H_
#define PLUGINS_USBDMX_USBDMXPLUGIN_H_

#include <set>
#include <string>
#include <utility>
#include <vector>
#include "ola/plugin_id.h"
#include "olad/Plugin.h"
#include "ola/io/Descriptor.h"

namespace ola {
namespace plugin {

namespace usbdmx {

using ola::io::ConnectedDescriptor;

class UsbDmxPlugin: public ola::Plugin {
 public:
    explicit UsbDmxPlugin(PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_anyma_devices_missing_serial_numbers(false) {
    }

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_USBDMX; }
    string PluginPrefix() const { return PLUGIN_PREFIX; }

    bool AddDeviceDescriptor(int fd);
    bool RemoveDeviceDescriptor(int fd);
    void SocketReady();

 private:
    struct USBDeviceInformation {
      string manufacturer;
      string product;
      string serial;
    };

    bool m_anyma_devices_missing_serial_numbers;
    vector<class UsbDevice*> m_devices;  // list of our devices
    vector<ola::io::DeviceDescriptor*> m_descriptors;
    set<std::pair<uint8_t, uint8_t> > m_registered_devices;

    bool StartHook();
    bool LoadFirmware();
    void FindDevices();
    bool StopHook();
    bool SetDefaultPreferences();
    class UsbDevice* NewAnymaDevice(
        struct libusb_device *usb_device,
        const struct libusb_device_descriptor &device_descriptor);

    void GetDeviceInfo(
        struct libusb_device_handle *usb_handle,
        const struct libusb_device_descriptor &device_descriptor,
        USBDeviceInformation *device_info);
    bool MatchManufacturer(const string &expected, const string &actual);
    bool MatchProduct(const string &expected, const string &actual);
    bool GetDescriptorString(libusb_device_handle *usb_handle,
                             uint8_t desc_index,
                             string *data);

    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char LIBUSB_DEBUG_LEVEL_KEY[];
    static int LIBUSB_DEFAULT_DEBUG_LEVEL;
    static int LIBUSB_MAX_DEBUG_LEVEL;
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_USBDMXPLUGIN_H_
