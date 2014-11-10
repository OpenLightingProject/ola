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
 * AnymaDeviceManager.cpp
 * The Anyma Device Manager
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/AnymaDeviceManager.h"

#include "ola/Logging.h"
#include "plugins/usbdmx/AnymaDevice.h"
#include "plugins/usbdmx/AnymaWidget.h"
#include "plugins/usbdmx/LibUsbHelper.h"

namespace ola {
namespace plugin {
namespace usbdmx {

const uint16_t AnymaDeviceManager::ANYMA_VENDOR_ID = 0x16C0;

bool AnymaDeviceManager::DeviceAdded(
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != ANYMA_VENDOR_ID ||
      descriptor.idProduct != 0x05DC ||
      HasDevice(usb_device)) {
    return false;
  }

  OLA_INFO << "Found a new Anyma device";
  LibUsbHelper::DeviceInformation info;
  if (!LibUsbHelper::GetDeviceInfo(usb_device, descriptor, &info)) {
    return false;
  }

  if (!LibUsbHelper::CheckManufacturer(
        AnymaWidgetInterface::EXPECTED_MANUFACTURER, info.manufacturer)) {
    return false;
  }

  if (!LibUsbHelper::CheckProduct(AnymaWidgetInterface::EXPECTED_PRODUCT,
                                  info.product)) {
    return false;
  }

  if (info.serial.empty()) {
    if (m_missing_serial_number) {
      OLA_WARN << "Failed to read serial number or serial number empty. "
               << "We can only support one device without a serial number.";
      return false;
    } else {
      OLA_WARN << "Failed to read serial number from " << info.manufacturer
               << " : " << info.product
               << " the device probably doesn't have one";
      m_missing_serial_number = true;
    }
  }

  AsynchronousAnymaWidget *widget = new AsynchronousAnymaWidget(usb_device);
  if (!widget->Init()) {
    delete widget;
    return false;
  }

  AnymaDevice *device = new AnymaDevice(ParentPlugin(), widget, info.serial);
  return RegisterDevice(usb_device, device);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
