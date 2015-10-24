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
 * EuroliteProFactory.cpp
 * The WidgetFactory for EurolitePro widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/EuroliteProFactory.h"

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"
#include "ola/base/Flags.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::LibUsbAdaptor;

const char EuroliteProFactory::EXPECTED_MANUFACTURER[] = "Eurolite";
const char EuroliteProFactory::EXPECTED_PRODUCT[] = "Eurolite DMX512 Pro";
const uint16_t EuroliteProFactory::PRODUCT_ID = 0xfa63;
const uint16_t EuroliteProFactory::VENDOR_ID = 0x04d8;

bool EuroliteProFactory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != VENDOR_ID || descriptor.idProduct != PRODUCT_ID) {
    return false;
  }

  OLA_INFO << "Found a new EurolitePro device";
  LibUsbAdaptor::DeviceInformation info;
  if (!m_adaptor->GetDeviceInfo(usb_device, descriptor, &info)) {
    return false;
  }

  if (!m_adaptor->CheckManufacturer(EXPECTED_MANUFACTURER, info)) {
    return false;
  }

  if (!m_adaptor->CheckProduct(EXPECTED_PRODUCT, info)) {
    return false;
  }

  // The Eurolite doesn't have a serial number, so instead we use the device &
  // bus number.
  // TODO(simon): check if this supports the SERIAL NUMBER label and use that
  // instead.

  // There is no Serialnumber--> work around: bus+device number
  int bus_number = libusb_get_bus_number(usb_device);
  int device_address = libusb_get_device_address(usb_device);

  std::ostringstream serial_str;
  serial_str << bus_number << "-" << device_address;

  EurolitePro *widget = NULL;
  if (FLAGS_use_async_libusb) {
    widget = new AsynchronousEurolitePro(m_adaptor, usb_device,
                                         serial_str.str());
  } else {
    widget = new SynchronousEurolitePro(m_adaptor, usb_device,
                                        serial_str.str());
  }
  return AddWidget(observer, widget);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
