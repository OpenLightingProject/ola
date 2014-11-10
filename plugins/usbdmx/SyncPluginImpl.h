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
 * SyncPluginImpl.h
 * The synchronous implementation of the USB DMX plugin.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBDMX_SYNCPLUGINIMPL_H_
#define PLUGINS_USBDMX_SYNCPLUGINIMPL_H_

#include <libusb.h>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "ola/base/Macro.h"
#include "plugins/usbdmx/PluginImplInterface.h"
#include "plugins/usbdmx/LibUsbAdaptor.h"


namespace ola {

class Device;

namespace plugin {
namespace usbdmx {

/**
 * @brief The legacy implementation.
 *
 * This implementation spawns a thread for each dongle, and then uses
 * synchronous calls to libusb.
 *
 * This does not support hotplug.
 */
class SyncPluginImpl: public PluginImplInterface {
 public:
  /**
   * @brief Create a new SyncPluginImpl.
   * @param plugin_adaptor The PluginAdaptor to use, ownership is not
   * transferred.
   * @param plugin The parent Plugin object which is used when creating
   * devices.
   * @param libusb_adaptor The adaptor to use when calling libusb.
   */
  SyncPluginImpl(PluginAdaptor *plugin_adaptor,
                 Plugin *plugin,
                 LibUsbAdaptor *libusb_adaptor);

  bool Start();
  bool Stop();

 private:
  struct USBDeviceInformation {
    std::string manufacturer;
    std::string product;
    std::string serial;
  };

  PluginAdaptor *m_plugin_adaptor;
  Plugin *m_plugin;
  LibUsbAdaptor *m_libusb_adaptor;

  libusb_context *m_context;

  bool m_anyma_devices_missing_serial_numbers;
  std::vector<class Device*> m_devices;  // list of our devices
  std::set<std::pair<uint8_t, uint8_t> > m_registered_devices;

  bool LoadFirmware();
  void FindDevices();
  void CheckDevice(libusb_device *device);

  class Device* NewAnymaDevice(
      struct libusb_device *usb_device,
      const struct libusb_device_descriptor &device_descriptor);

  void GetDeviceInfo(
      struct libusb_device_handle *usb_handle,
      const struct libusb_device_descriptor &device_descriptor,
      USBDeviceInformation *device_info);
  bool MatchManufacturer(const std::string &expected,
                         const std::string &actual);
  bool MatchProduct(const std::string &expected, const std::string &actual);
  bool GetDescriptorString(libusb_device_handle *usb_handle,
                           uint8_t desc_index,
                           std::string *data);

  DISALLOW_COPY_AND_ASSIGN(SyncPluginImpl);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SYNCPLUGINIMPL_H_
