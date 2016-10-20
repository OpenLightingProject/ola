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
 * DMXCreator512BasicFactory.h
 * The factory for DMXCreator512Basic widgets.
 * Copyright (C) 2016 Florian Edelmann
 */

#ifndef PLUGINS_USBDMX_DMXCREATOR512BASICFACTORY_H_
#define PLUGINS_USBDMX_DMXCREATOR512BASICFACTORY_H_

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/base/Macro.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Creates DMXCreator512Basic widgets.
 */
class DMXCreator512BasicFactory
    : public BaseWidgetFactory<class DMXCreator512Basic> {
 public:
  explicit DMXCreator512BasicFactory(ola::usb::LibUsbAdaptor *adaptor)
      : BaseWidgetFactory<class DMXCreator512Basic>(
            "DMXCreator512BasicFactory"),
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

  static const uint16_t PRODUCT_ID;
  static const uint16_t VENDOR_ID;

  DISALLOW_COPY_AND_ASSIGN(DMXCreator512BasicFactory);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_DMXCREATOR512BASICFACTORY_H_
