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
 * EuroliteProFactory.h
 * The WidgetFactory for EurolitePro widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_EUROLITEPROFACTORY_H_
#define PLUGINS_USBDMX_EUROLITEPROFACTORY_H_

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/base/Macro.h"
#include "olad/Preferences.h"
#include "plugins/usbdmx/EurolitePro.h"
#include "plugins/usbdmx/WidgetFactory.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief Creates EurolitePro widgets.
 */
class EuroliteProFactory : public BaseWidgetFactory<class EurolitePro> {
 public:
  explicit EuroliteProFactory(ola::usb::LibUsbAdaptor *adaptor,
                              Preferences *preferences);

  bool DeviceAdded(WidgetObserver *observer,
                   libusb_device *usb_device,
                   const struct libusb_device_descriptor &descriptor);

  static bool IsEuroliteMk2Enabled(Preferences *preferences);

 private:
  ola::usb::LibUsbAdaptor *m_adaptor;
  bool m_enable_eurolite_mk2;

  static const uint16_t PRODUCT_ID;
  static const uint16_t VENDOR_ID;
  static const char EXPECTED_MANUFACTURER[];
  static const char EXPECTED_PRODUCT[];

  static const uint16_t PRODUCT_ID_MK2;
  static const uint16_t VENDOR_ID_MK2;
  static const char EXPECTED_MANUFACTURER_MK2[];
  static const char EXPECTED_PRODUCT_MK2[];

  static const char ENABLE_EUROLITE_MK2_KEY[];

  DISALLOW_COPY_AND_ASSIGN(EuroliteProFactory);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_EUROLITEPROFACTORY_H_
