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
 * JaRuleFactory.h
 * The factory for Ja Rule widgets.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef PLUGINS_USBDMX_JARULEFACTORY_H_
#define PLUGINS_USBDMX_JARULEFACTORY_H_

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/base/Macro.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/rdm/UID.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {
namespace usb {
class JaRuleWidget;
}  // namespace usb

namespace plugin {
namespace usbdmx {

/**
 * @brief Creates Ja Rule widgets.
 */
class JaRuleFactory : public BaseWidgetFactory<ola::usb::JaRuleWidget> {
 public:
  explicit JaRuleFactory(ola::io::SelectServerInterface *ss,
                         ola::usb::AsynchronousLibUsbAdaptor *adaptor)
      : BaseWidgetFactory<ola::usb::JaRuleWidget>("JaRuleFactory"),
        m_ss(ss),
        m_adaptor(adaptor) {
  }

  bool DeviceAdded(WidgetObserver *observer,
                   libusb_device *usb_device,
                   const struct libusb_device_descriptor &descriptor);

 private:
  ola::io::SelectServerInterface *m_ss;
  ola::usb::AsynchronousLibUsbAdaptor *m_adaptor;

  static const uint16_t PRODUCT_ID;
  static const uint16_t VENDOR_ID;

  DISALLOW_COPY_AND_ASSIGN(JaRuleFactory);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_JARULEFACTORY_H_
