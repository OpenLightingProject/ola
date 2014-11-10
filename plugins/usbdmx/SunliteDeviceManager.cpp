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
 * SunliteDeviceManager.cpp
 * The Sunlite Device Manager
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/SunliteDeviceManager.h"
#include "plugins/usbdmx/SunliteFirmwareLoader.h"
#include "plugins/usbdmx/SunliteWidget.h"

#include "ola/Logging.h"

namespace ola {
namespace plugin {
namespace usbdmx {

const uint16_t SunliteDeviceManager::SUNLITE_VENDOR_ID = 0x0962;

bool SunliteDeviceManager::DeviceAdded(
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor == SUNLITE_VENDOR_ID &&
      descriptor.idProduct == 0x2000) {
    OLA_INFO << "New empty SunliteDevice";
    // TODO(simon): Make this async.
    SunliteFirmwareLoader loader(usb_device);
    loader.LoadFirmware();
    return true;
  } else if (descriptor.idVendor == SUNLITE_VENDOR_ID &&
             descriptor.idProduct == 0x2001 &&
             !HasDevice(usb_device)) {
    OLA_INFO << "Found a new Sunlite device";


    AsynchronousSunliteWidget *widget = new AsynchronousSunliteWidget(
      usb_device);
    if (!widget->Init()) {
      delete widget;
      return false;
    }
    SunliteDevice *device = new SunliteDevice(ParentPlugin(), widget);
    return RegisterDevice(usb_device, device);
  }
  return false;
}

void SunliteDeviceManager::DeviceRemoved(libusb_device *device) {
  // TODO(simon): once firmware loading is async, cancel the load here.
  BaseDeviceFactory<SunliteDevice>::DeviceRemoved(device);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
