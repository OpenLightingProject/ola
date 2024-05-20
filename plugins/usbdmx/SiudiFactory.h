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
 * SiudiFactory.h
 * The WidgetFactory for SIUDI widgets.
 * Copyright (C) 2023 Alexander Simon
 */

#ifndef PLUGINS_USBDMX_SIUDIFACTORY_H_
#define PLUGINS_USBDMX_SIUDIFACTORY_H_

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/base/Macro.h"
#include "plugins/usbdmx/Siudi.h"
#include "plugins/usbdmx/Siudi.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Creates SunLite widgets.
 */
class SiudiFactory : public BaseWidgetFactory<Siudi> {
 public:
  explicit SiudiFactory(ola::usb::LibUsbAdaptor *adaptor)
      : BaseWidgetFactory<Siudi>("SiudiFactory"),
        m_adaptor(adaptor) {}

  bool DeviceAdded(
      WidgetObserver *observer,
      libusb_device *usb_device,
      const struct libusb_device_descriptor &descriptor);

 private:
  ola::usb::LibUsbAdaptor* const m_adaptor;

  static const uint16_t NICOLAUDIE_ID;
  // The product ID for widgets that are missing their firmware.
  static const uint16_t SIUDI6_COLD_ID;
  // The product ID for widgets with the firmware.
  static const uint16_t SIUDI6C_HOT_ID;
  static const uint16_t SIUDI6A_HOT_ID;
  static const uint16_t SIUDI6D_HOT_ID;

  DISALLOW_COPY_AND_ASSIGN(SiudiFactory);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SIUDIFACTORY_H_
