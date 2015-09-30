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
 * SunliteFactory.cpp
 * The WidgetFactory for SunLite widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/SunliteFactory.h"

#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "plugins/usbdmx/SunliteFirmwareLoader.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

const uint16_t SunliteFactory::EMPTY_PRODUCT_ID = 0x2000;
const uint16_t SunliteFactory::FULL_PRODUCT_ID = 0x2001;
const uint16_t SunliteFactory::VENDOR_ID = 0x0962;

bool SunliteFactory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor == VENDOR_ID &&
      descriptor.idProduct == EMPTY_PRODUCT_ID) {
    OLA_INFO << "New empty SunliteDevice";
    // TODO(simon): Make this async.
    SunliteFirmwareLoader loader(usb_device);
    loader.LoadFirmware();
    return true;
  } else if (descriptor.idVendor == VENDOR_ID &&
             descriptor.idProduct == FULL_PRODUCT_ID) {
    OLA_INFO << "Found a new Sunlite device";
    Sunlite *widget = NULL;
    if (FLAGS_use_async_libusb) {
      widget = new AsynchronousSunlite(m_adaptor, usb_device);
    } else {
      widget = new SynchronousSunlite(m_adaptor, usb_device);
    }
    return AddWidget(observer, widget);
  }
  return false;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
