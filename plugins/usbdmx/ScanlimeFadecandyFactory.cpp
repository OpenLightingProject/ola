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
 * ScanlimeFadecandyFactory.cpp
 * The WidgetFactory for Fadecandy widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/ScanlimeFadecandyFactory.h"

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "plugins/usbdmx/ScanlimeFadecandy.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::LibUsbAdaptor;

const char ScanlimeFadecandyFactory::EXPECTED_MANUFACTURER[] = "scanlime";
const char ScanlimeFadecandyFactory::EXPECTED_PRODUCT[] = "Fadecandy";
const uint16_t ScanlimeFadecandyFactory::VENDOR_ID = 0x1D50;
const uint16_t ScanlimeFadecandyFactory::PRODUCT_ID = 0x607A;


bool ScanlimeFadecandyFactory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != VENDOR_ID || descriptor.idProduct != PRODUCT_ID) {
    return false;
  }

  OLA_INFO << "Found a new Fadecandy device";
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

  // Fadecandy devices may be missing serial numbers. Since there isn't another
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

  ScanlimeFadecandy *widget = NULL;
  if (FLAGS_use_async_libusb) {
    widget = new AsynchronousScanlimeFadecandy(m_adaptor, usb_device,
                                               info.serial);
  } else {
    widget = new SynchronousScanlimeFadecandy(m_adaptor, usb_device,
                                              info.serial);
  }
  return AddWidget(observer, widget);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
