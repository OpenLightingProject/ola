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
 * SiudiFactory.cpp
 * The WidgetFactory for SIUDI widgets.
 * Copyright (C) 2023 Alexander Simon
 */

#include "plugins/usbdmx/SiudiFactory.h"

#include "ola/Logging.h"
#include "ola/base/Flags.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

const uint16_t SiudiFactory::NICOLAUDIE_ID = 0x6244;
const uint16_t SiudiFactory::SIUDI6_COLD_ID = 0x0300;
const uint16_t SiudiFactory::SIUDI6C_HOT_ID = 0x301;
const uint16_t SiudiFactory::SIUDI6A_HOT_ID = 0x302;
const uint16_t SiudiFactory::SIUDI6D_HOT_ID = 0x303;

bool SiudiFactory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != NICOLAUDIE_ID) {
    return false;
  }
  if (descriptor.idProduct == SIUDI6_COLD_ID) {
    OLA_WARN << "Found a Nicoleaudie SIUDI-6 device in cold state. "
                "Firmware download is currently not supported.";
    return false;
  }
  if (descriptor.idProduct == SIUDI6C_HOT_ID ||
      descriptor.idProduct == SIUDI6A_HOT_ID ||
      descriptor.idProduct == SIUDI6D_HOT_ID) {
    if (descriptor.idProduct == SIUDI6C_HOT_ID) {
      OLA_INFO << "Found a new Nicoleaudie SIUDI-6C device";
    } else if (descriptor.idProduct == SIUDI6A_HOT_ID) {
      OLA_INFO << "Found a new Nicoleaudie SIUDI-6A device";
    } else if (descriptor.idProduct == SIUDI6D_HOT_ID) {
      OLA_INFO << "Found a new Nicoleaudie SIUDI-6D device";
    }
    Siudi *widget = NULL;
    widget = new SynchronousSiudi(m_adaptor, usb_device);
    return AddWidget(observer, widget);
  }
  return false;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
