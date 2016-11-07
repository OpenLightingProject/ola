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
 * ShowJockeyFactory.h
 * The WidgetFactory for ShowJockey widgets.
 * Copyright (C) 2016 Nicolas Bertrand
 */

#ifndef PLUGINS_USBDMX_SHOWJOCKEYFACTORY_H_
#define PLUGINS_USBDMX_SHOWJOCKEYFACTORY_H_

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/base/Macro.h"
#include "plugins/usbdmx/ShowJockey.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Creates ShowJockey widgets.
 */
class ShowJockeyFactory : public BaseWidgetFactory<class ShowJockey> {
 public:
  explicit ShowJockeyFactory(ola::usb::LibUsbAdaptor *adaptor)
      : BaseWidgetFactory<class ShowJockey>("ShowJockeyFactory"),
        m_adaptor(adaptor) {}

  bool DeviceAdded(WidgetObserver *observer,
                   libusb_device *usb_device,
                   const struct libusb_device_descriptor &descriptor);

 private:
  ola::usb::LibUsbAdaptor *m_adaptor;

  static const uint16_t PRODUCT_ID;
  static const uint16_t VENDOR_ID;


  DISALLOW_COPY_AND_ASSIGN(ShowJockeyFactory);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SHOWJOCKEYFACTORY_H_
