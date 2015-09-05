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
 * NodleU1Factory.cpp
 * The WidgetFactory for Nodle widgets.
 * Copyright (C) 2015 Stefan Krupop
 */

#include "plugins/usbdmx/NodleU1Factory.h"

#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "plugins/usbdmx/NodleU1.h"
#include "plugins/usbdmx/LibUsbAdaptor.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

const uint16_t NodleU1Factory::VENDOR_ID = 0x16d0;
const uint16_t NodleU1Factory::PRODUCT_ID = 0x0830;

bool NodleU1Factory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != VENDOR_ID || descriptor.idProduct != PRODUCT_ID ||
      HasDevice(usb_device)) {
    return false;
  }

  OLA_INFO << "Found a new Nodle U1 device";
  LibUsbAdaptor::DeviceInformation info;
  if (!m_adaptor->GetDeviceInfo(usb_device, descriptor, &info)) {
    return false;
  }

  OLA_INFO << "Nodle U1 serial: " << info.serial;

  unsigned int mode;
  if (!StringToInt(m_preferences->GetValue(NodleU1::NODLE_MODE_KEY) ,
                   &mode)) {
    mode = NodleU1::NODLE_DEFAULT_MODE;
  }

  OLA_INFO << "Setting Nodle U1 mode to " << mode;

  NodleU1 *widget = NULL;
  if (FLAGS_use_async_libusb) {
    widget = new AsynchronousNodleU1(m_adaptor, usb_device, info.serial, mode);
  } else {
    widget = new SynchronousNodleU1(m_adaptor, usb_device, info.serial, mode);
  }
  return AddWidget(observer, usb_device, widget);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
