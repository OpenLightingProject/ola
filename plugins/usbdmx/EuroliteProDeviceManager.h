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
 * EuroliteProDeviceManager.h
 * The EurolitePro Device Manager.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_EUROLITEPRODEVICEMANAGER_H_
#define PLUGINS_USBDMX_EUROLITEPRODEVICEMANAGER_H_

#include "ola/base/Macro.h"
#include "plugins/usbdmx/UsbDeviceManagerInterface.h"
#include "plugins/usbdmx/EuroliteProDevice.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Manages EurolitePro Devices
 */
class EuroliteProDeviceManager : public BaseDeviceFactory<EuroliteProDevice> {
 public:
  EuroliteProDeviceManager(PluginAdaptor *plugin_adaptor,
                     Plugin *plugin)
      : BaseDeviceFactory<EuroliteProDevice>(plugin_adaptor, plugin) {
  }

  bool DeviceAdded(
    libusb_device *device,
    const struct libusb_device_descriptor &descriptor);

 private:
  static const uint16_t EUROLITE_PRODUCT_ID;
  static const uint16_t EUROLITE_VENDOR_ID;

  DISALLOW_COPY_AND_ASSIGN(EuroliteProDeviceManager);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_EUROLITEPRODEVICEMANAGER_H_
