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
 * AnymaDeviceManager.h
 * The Anyma Device Manager.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_ANYMADEVICEMANAGER_H_
#define PLUGINS_USBDMX_ANYMADEVICEMANAGER_H_

#include "ola/base/Macro.h"
#include "plugins/usbdmx/UsbDeviceManagerInterface.h"
#include "plugins/usbdmx/AnymaDevice.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Manages Anyma Devices
 */
class AnymaDeviceManager : public BaseDeviceFactory<AnymaDevice> {
 public:
  AnymaDeviceManager(PluginAdaptor *plugin_adaptor,
                     Plugin *plugin)
      : BaseDeviceFactory<AnymaDevice>(plugin_adaptor, plugin),
        m_missing_serial_number(false) {}

  bool DeviceAdded(
    libusb_device *device,
    const struct libusb_device_descriptor &descriptor);

 private:
  // Some Anyma devices don't have serial numbers. Since there isn't another
  // good way to uniquely identify a USB device, we only support one of these
  // types of devices.
  bool m_missing_serial_number;

  static const uint16_t ANYMA_VENDOR_ID;

  DISALLOW_COPY_AND_ASSIGN(AnymaDeviceManager);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_ANYMADEVICEMANAGER_H_
