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
 * USBDMXComFactory.h
 * The WidgetFactory for USBDMXCom widgets.
 * Copyright (C) 2021 Peter Newman
 */

#ifndef PLUGINS_USBDMX_USBDMXCOMFACTORY_H_
#define PLUGINS_USBDMX_USBDMXCOMFACTORY_H_

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/base/Macro.h"
#include "olad/Preferences.h"
#include "plugins/usbdmx/USBDMXCom.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Creates USBDMXCom widgets.
 */
class USBDMXComFactory : public BaseWidgetFactory<class USBDMXCom> {
 public:
  explicit USBDMXComFactory(ola::usb::LibUsbAdaptor *adaptor,
                            Preferences *preferences);

  bool DeviceAdded(WidgetObserver *observer,
                   libusb_device *usb_device,
                   const struct libusb_device_descriptor &descriptor);

  static bool IsUSBDMXComEnabled(Preferences *preferences);

  static const char ENABLE_USBDMXCOM_KEY[];

 private:
  ola::usb::LibUsbAdaptor *m_adaptor;
  bool m_enable_usbdmxcom;

  static const uint16_t PRODUCT_ID;
  static const uint16_t VENDOR_ID;
  static const char EXPECTED_MANUFACTURER[];
  static const char EXPECTED_PRODUCT[];

  DISALLOW_COPY_AND_ASSIGN(USBDMXComFactory);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_USBDMXCOMFACTORY_H_
