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
 * DMXCProjectsNodleU1Factory.cpp
 * The WidgetFactory for Nodle widgets.
 * Copyright (C) 2015 Stefan Krupop
 */

#include "plugins/usbdmx/DMXCProjectsNodleU1Factory.h"

#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "plugins/usbdmx/DMXCProjectsNodleU1.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

const uint16_t DMXCProjectsNodleU1Factory::VENDOR_ID_DMXC_PROJECTS = 0x16d0;
const uint16_t DMXCProjectsNodleU1Factory::PRODUCT_ID_DMXC_P_NODLE_U1 = 0x0830;

const uint16_t DMXCProjectsNodleU1Factory::VENDOR_ID_DE = 0x4b4;
const uint16_t DMXCProjectsNodleU1Factory::PRODUCT_ID_DE_USB_DMX = 0xf1f;

const uint16_t DMXCProjectsNodleU1Factory::VENDOR_ID_FX5 = 0x16c0;
const uint16_t DMXCProjectsNodleU1Factory::PRODUCT_ID_FX5_DMX = 0x88b;

bool DMXCProjectsNodleU1Factory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  if (!((descriptor.idVendor == VENDOR_ID_DMXC_PROJECTS &&
         descriptor.idProduct == PRODUCT_ID_DMXC_P_NODLE_U1) ||
        (descriptor.idVendor == VENDOR_ID_DE &&
         descriptor.idProduct == PRODUCT_ID_DE_USB_DMX) ||
        (descriptor.idVendor == VENDOR_ID_FX5 &&
         descriptor.idProduct == PRODUCT_ID_FX5_DMX) )) {
    return false;
  }

  OLA_INFO << "Found a new Nodle U1 device";
  ola::usb::LibUsbAdaptor::DeviceInformation info;
  if (!m_adaptor->GetDeviceInfo(usb_device, descriptor, &info)) {
    return false;
  }

  OLA_INFO << "Nodle U1 serial: " << info.serial;

  if (m_preferences->SetDefaultValue(
      "nodle-" + info.serial + "-mode",
      UIntValidator(DMXCProjectsNodleU1::NODLE_MIN_MODE,
                    DMXCProjectsNodleU1::NODLE_MAX_MODE),
      DMXCProjectsNodleU1::NODLE_DEFAULT_MODE)) {
    m_preferences->Save();
  }

  unsigned int mode;
  if (!StringToInt(m_preferences->GetValue("nodle-" + info.serial + "-mode"),
                   &mode)) {
    mode = DMXCProjectsNodleU1::NODLE_DEFAULT_MODE;
  }

  OLA_INFO << "Setting Nodle U1 mode to " << mode;

  DMXCProjectsNodleU1 *widget = NULL;
  if (FLAGS_use_async_libusb) {
    widget = new AsynchronousDMXCProjectsNodleU1(m_adaptor, usb_device,
                                                 m_plugin_adaptor, info.serial,
                                                 mode);
  } else {
    widget = new SynchronousDMXCProjectsNodleU1(m_adaptor, usb_device,
                                                m_plugin_adaptor, info.serial,
                                                mode);
  }
  return AddWidget(observer, widget);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
