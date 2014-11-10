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
 * SunliteDeviceManager.h
 * The Sunlite Device Manager
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_SUNLITEDEVICEMANAGER_H_
#define PLUGINS_USBDMX_SUNLITEDEVICEMANAGER_H_

#include "ola/base/Macro.h"
#include "plugins/usbdmx/UsbDeviceManagerInterface.h"
#include "plugins/usbdmx/SunliteDevice.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Manages SunLite Devices
 */
class SunliteDeviceManager : public BaseDeviceFactory<SunliteDevice> {
 public:
  SunliteDeviceManager(PluginAdaptor *plugin_adaptor,
                       Plugin *plugin)
      : BaseDeviceFactory<SunliteDevice>(plugin_adaptor, plugin) {}

  bool DeviceAdded(libusb_device *device,
                   const struct libusb_device_descriptor &descriptor);

  void DeviceRemoved(libusb_device *device);

 private:
  static const uint16_t SUNLITE_VENDOR_ID;

  DISALLOW_COPY_AND_ASSIGN(SunliteDeviceManager);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SUNLITEDEVICEMANAGER_H_
