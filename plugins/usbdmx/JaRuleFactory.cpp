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
 * JaRuleFactory.cpp
 * The factory for Ja Rule widgets.
 * Copyright (C) 2015 Simon Newton
 */

#include "plugins/usbdmx/JaRuleFactory.h"

#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "plugins/usbdmx/JaRuleWidget.h"
#include "plugins/usbdmx/LibUsbAdaptor.h"

DECLARE_bool(use_async_libusb);

DEFINE_string(ja_rule_controller_uid, "7a70:fffffe00",
              "The UID of the Ja Rule controller.");

namespace ola {
namespace plugin {
namespace usbdmx {

// http://pid.codes/1209/ACED/
const uint16_t JaRuleFactory::PRODUCT_ID = 0xaced;
const uint16_t JaRuleFactory::VENDOR_ID = 0x1209;

bool JaRuleFactory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (descriptor.idVendor != VENDOR_ID || descriptor.idProduct != PRODUCT_ID ||
      HasDevice(usb_device)) {
    return false;
  }

  OLA_INFO << "Found a new Ja Rule device";
  LibUsbAdaptor::DeviceInformation info;
  if (!m_adaptor->GetDeviceInfo(usb_device, descriptor, &info)) {
    return false;
  }

  // TODO(simon): Support multiple widgets.
  if (DeviceCount() > 0) {
    OLA_WARN << "Only a single Ja Rule device is supported";
    return false;
  }

  if (FLAGS_use_async_libusb) {
    return AddWidget(observer, usb_device,
                     new JaRuleWidget(m_ss, m_adaptor, usb_device, m_uid));
  } else {
    OLA_WARN << "Ja Rule devices are not supported in Synchronous mode";
    return false;
  }
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
