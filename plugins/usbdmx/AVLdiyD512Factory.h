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
 * AVLdiyD512Factory.h
 * The factory for AVLdiy D512 widgets.
 * Copyright (C) 2016 Tor Håkon Gjerde
 */

#ifndef PLUGINS_USBDMX_AVLDIYD512FACTORY_H_
#define PLUGINS_USBDMX_AVLDIYD512FACTORY_H_

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/base/Macro.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Creates AVLdiy widgets.
 */
class AVLdiyD512Factory : public BaseWidgetFactory<class AVLdiyD512> {
 public:
  explicit AVLdiyD512Factory(ola::usb::LibUsbAdaptor *adaptor)
      : BaseWidgetFactory<class AVLdiyD512>("AVLdiyD512Factory"),
        m_missing_serial_number(false),
        m_adaptor(adaptor) {
  }

  bool DeviceAdded(
      WidgetObserver *observer,
      libusb_device *usb_device,
      const struct libusb_device_descriptor &descriptor);

 private:
  bool m_missing_serial_number;
  ola::usb::LibUsbAdaptor *m_adaptor;

  static const char EXPECTED_MANUFACTURER[];
  static const char EXPECTED_PRODUCT[];
  static const uint16_t PRODUCT_ID;
  static const uint16_t VENDOR_ID;

  DISALLOW_COPY_AND_ASSIGN(AVLdiyD512Factory);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_AVLDIYD512FACTORY_H_
