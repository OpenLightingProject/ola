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
 * VellemanK8062Factory.cpp
 * The WidgetFactory for Velleman widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/VellemanK8062Factory.h"

#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "plugins/usbdmx/VellemanK8062.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

const uint16_t VellemanK8062Factory::VENDOR_ID = 0x10cf;
const uint16_t VellemanK8062Factory::PRODUCT_ID = 0x8062;

bool VellemanK8062Factory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != VENDOR_ID || descriptor.idProduct != PRODUCT_ID) {
    return false;
  }

  OLA_INFO << "Found a new Velleman device";
  VellemanK8062 *widget = NULL;
  if (FLAGS_use_async_libusb) {
    widget = new AsynchronousVellemanK8062(m_adaptor, usb_device);
  } else {
    widget = new SynchronousVellemanK8062(m_adaptor, usb_device);
  }
  return AddWidget(observer, widget);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
