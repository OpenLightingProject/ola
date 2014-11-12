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
 * EuroliteProWidgetFactory.cpp
 * The WidgetFactory for EurolitePro widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/EuroliteProWidgetFactory.h"

#include "ola/Logging.h"
#include "plugins/usbdmx/LibUsbHelper.h"

namespace ola {
namespace plugin {
namespace usbdmx {

const uint16_t EuroliteProWidgetFactory::EUROLITE_PRODUCT_ID = 0xfa63;
const uint16_t EuroliteProWidgetFactory::EUROLITE_VENDOR_ID = 0x04d;

bool EuroliteProWidgetFactory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != EUROLITE_VENDOR_ID ||
      descriptor.idProduct != EUROLITE_PRODUCT_ID ||
      HasDevice(usb_device)) {
    return false;
  }

  OLA_INFO << "Found a new EurolitePro device";
  LibUsbHelper::DeviceInformation info;
  if (!LibUsbHelper::GetDeviceInfo(usb_device, descriptor, &info)) {
    return false;
  }

  if (!LibUsbHelper::CheckManufacturer(
        EuroliteProWidget::EXPECTED_MANUFACTURER, info.manufacturer)) {
    return false;
  }

  if (!LibUsbHelper::CheckProduct(
        EuroliteProWidget::EXPECTED_PRODUCT, info.product)) {
    return false;
  }

  // The Eurolite doesn't have a serial number, so instead we use the device &
  // bus number.
  // TODO(simon): check if this supports the SERIAL NUMBER label and use that
  // instead.

  // There is no Serialnumber--> work around: bus+device number
  int bus_number = libusb_get_bus_number(usb_device);
  int device_address = libusb_get_device_address(usb_device);

  OLA_INFO << "Bus_number: " <<  bus_number << ", Device_address: " <<
    device_address;

  std::ostringstream serial_str;
  serial_str << bus_number << "-" << device_address;

  return AddWidget(
      observer,
      usb_device,
      new AsynchronousEuroliteProWidget(usb_device, serial_str.str()));
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
