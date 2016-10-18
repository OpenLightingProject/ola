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
 * DMXCreatorFactory.cpp
 * The factory for DMXCreator widgets.
 * Copyright (C) 2016 Florian Edelmann
 */

#include "plugins/usbdmx/DMXCreatorFactory.h"

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "plugins/usbdmx/DMXCreator.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::LibUsbAdaptor;

const uint16_t DMXCreatorFactory::VENDOR_ID = 0x0a30;
const uint16_t DMXCreatorFactory::PRODUCT_ID = 0x0002;

bool DMXCreatorFactory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != VENDOR_ID || descriptor.idProduct != PRODUCT_ID) {
    return false;
  }

  OLA_INFO << "Found a new DMXCreator device";
  // Unfortunately, DMXCreator devices don't provide any additional information
  // that identify them. So we have to stick with just testing vendor and
  // product ids. Also, since DMXCreator devices don't have serial numbers and
  // there is no other good way to uniquely identify a USB device, we only
  // support one of these types of devices per host.
  if (m_missing_serial_number) {
    OLA_WARN << "We can only support one device without a serial number.";
    return false;
  } else {
    m_missing_serial_number = true;
  }

  DMXCreator *widget = NULL;
  if (FLAGS_use_async_libusb) {
    widget = new AsynchronousDMXCreator(m_adaptor, usb_device);
  } else {
    // Synchronous mode consumes way too much memory and eventually gets killed
    // Until this is fixed, disable synchronous mode.
    // widget = new SynchronousDMXCreator(m_adaptor, usb_device);
    return false;
  }
  return AddWidget(observer, widget);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
