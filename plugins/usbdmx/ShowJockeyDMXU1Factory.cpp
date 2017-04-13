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
 * ShowJockeyDMXU1Factory.cpp
 * The WidgetFactory for ShowJockeyDMXU1 widgets.
 * Copyright (C) 2016 Nicolas Bertrand
 */

#include "plugins/usbdmx/ShowJockeyDMXU1Factory.h"

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"
#include "ola/base/Flags.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::LibUsbAdaptor;

const char ShowJockeyDMXU1Factory::EXPECTED_MANUFACTURER[] =
    "Showjockey Co.,Ltd";
const char ShowJockeyDMXU1Factory::EXPECTED_PRODUCT[] =
    "Showjockey Co.,Ltd.USB TO DMX51";
const uint16_t ShowJockeyDMXU1Factory::PRODUCT_ID = 0x57fe;
const uint16_t ShowJockeyDMXU1Factory::VENDOR_ID = 0x0483;

bool ShowJockeyDMXU1Factory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != VENDOR_ID || descriptor.idProduct != PRODUCT_ID) {
    return false;
  }

  OLA_INFO << "Found a new ShowJockey device";
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

  ShowJockeyDMXU1 *widget = NULL;
  if (FLAGS_use_async_libusb) {
    widget = new AsynchronousShowJockeyDMXU1(m_adaptor, usb_device, info.serial);
  } else {
    widget = new SynchronousShowJockeyDMXU1(m_adaptor, usb_device, info.serial);
  }
  return AddWidget(observer, widget);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
