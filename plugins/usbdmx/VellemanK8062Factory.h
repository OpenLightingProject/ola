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
 * VellemanK8062Factory.h
 * The WidgetFactory for Velleman widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_VELLEMANK8062FACTORY_H_
#define PLUGINS_USBDMX_VELLEMANK8062FACTORY_H_

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/base/Macro.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Creates Velleman widgets.
 */
class VellemanK8062Factory : public BaseWidgetFactory<class VellemanK8062> {
 public:
  explicit VellemanK8062Factory(ola::usb::LibUsbAdaptor *adaptor)
      : BaseWidgetFactory<class VellemanK8062>("VellemanK8062Factory"),
        m_adaptor(adaptor) {
  }

  bool DeviceAdded(
      WidgetObserver *observer,
      libusb_device *usb_device,
      const struct libusb_device_descriptor &descriptor);

 private:
  ola::usb::LibUsbAdaptor* const m_adaptor;

  static const uint16_t VENDOR_ID;
  static const uint16_t PRODUCT_ID;

  DISALLOW_COPY_AND_ASSIGN(VellemanK8062Factory);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_VELLEMANK8062FACTORY_H_
