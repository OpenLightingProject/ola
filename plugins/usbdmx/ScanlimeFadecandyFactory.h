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
 * ScanlimeFadecandyFactory.h
 * The WidgetFactory for Scanlime FadeCandy widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_SCANLIMEFADECANDYFACTORY_H_
#define PLUGINS_USBDMX_SCANLIMEFADECANDYFACTORY_H_

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/base/Macro.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Creates Fadecandy widgets.
 */
class ScanlimeFadecandyFactory
    : public BaseWidgetFactory<class ScanlimeFadecandy> {
 public:
  explicit ScanlimeFadecandyFactory(ola::usb::LibUsbAdaptor *adaptor)
      : BaseWidgetFactory<class ScanlimeFadecandy>("ScanlimeFadecandyFactory"),
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

  DISALLOW_COPY_AND_ASSIGN(ScanlimeFadecandyFactory);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SCANLIMEFADECANDYFACTORY_H_
