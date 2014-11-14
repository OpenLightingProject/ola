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
 * VellemanWidgetFactory.cpp
 * The WidgetFactory for Velleman widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/VellemanWidgetFactory.h"

#include "ola/Logging.h"
#include "plugins/usbdmx/VellemanWidget.h"
#include "plugins/usbdmx/LibUsbHelper.h"

namespace ola {
namespace plugin {
namespace usbdmx {

const uint16_t VellemanWidgetFactory::VENDOR_ID = 0x10cf;
const uint16_t VellemanWidgetFactory::PRODUCT_ID = 0x8062;

bool VellemanWidgetFactory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != VENDOR_ID || descriptor.idProduct != PRODUCT_ID ||
      HasDevice(usb_device)) {
    return false;
  }

  OLA_INFO << "Found a new Velleman device";
  return AddWidget(observer, usb_device,
                   new AsynchronousVellemanWidget(usb_device));
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
