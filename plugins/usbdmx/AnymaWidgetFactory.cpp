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
 * AnymaWidgetFactory.cpp
 * The WidgetFactory for Anyma widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/AnymaWidgetFactory.h"

#include "ola/Logging.h"
#include "plugins/usbdmx/AnymaWidget.h"
#include "plugins/usbdmx/LibUsbHelper.h"

namespace ola {
namespace plugin {
namespace usbdmx {

const uint16_t AnymaWidgetFactory::VENDOR_ID = 0x16C0;
const uint16_t AnymaWidgetFactory::PRODUCT_ID = 0x05DC;

bool AnymaWidgetFactory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != VENDOR_ID || descriptor.idProduct != PRODUCT_ID ||
      HasDevice(usb_device)) {
    return false;
  }

  OLA_INFO << "Found a new Anyma device";
  LibUsbHelper::DeviceInformation info;
  if (!LibUsbHelper::GetDeviceInfo(usb_device, descriptor, &info)) {
    return false;
  }

  if (!LibUsbHelper::CheckManufacturer(
        AnymaWidget::EXPECTED_MANUFACTURER, info.manufacturer)) {
    return false;
  }

  if (!LibUsbHelper::CheckProduct(
        AnymaWidget::EXPECTED_PRODUCT, info.product)) {
    return false;
  }

  // Some Anyma devices don't have serial numbers. Since there isn't another
  // good way to uniquely identify a USB device, we only support one of these
  // types of devices per host.
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

  return AddWidget(observer, usb_device,
                   new AsynchronousAnymaWidget(usb_device, info.serial));
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
