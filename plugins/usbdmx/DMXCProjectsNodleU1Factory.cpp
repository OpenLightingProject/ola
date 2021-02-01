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

const uint16_t DMXCProjectsNodleU1Factory::VENDOR_ID = 0x16d0;
const uint16_t DMXCProjectsNodleU1Factory::PRODUCT_ID = 0x0830;

// Those IDs are not officially registered or "free for everyone"
// but there seems to be one clone using them.
// See also: https://github.com/mcallegari/qlcplus/blob/3b69452d0679333c6e60ff0928c20f2107ccc81f/plugins/hid/hiddmxdevice.h#L34
const uint16_t DMXCProjectsNodleU1Factory::VENDOR_ID_FX5 = 0x16c0;
const uint16_t DMXCProjectsNodleU1Factory::PRODUCT_ID_FX5 = 0x088b;

bool DMXCProjectsNodleU1Factory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  OLA_DEBUG << "Factory, DeviceAdded";
  if (
    ((descriptor.idVendor != VENDOR_ID) &&
    (descriptor.idVendor != VENDOR_ID_FX5)) ||
    ((descriptor.idProduct != PRODUCT_ID) &&
    (descriptor.idProduct != PRODUCT_ID_FX5))
    ) {
    return false;
  }

  OLA_INFO << "Found a new Nodle U1 or clone device";
  ola::usb::LibUsbAdaptor::DeviceInformation info;
  if (!m_adaptor->GetDeviceInfo(usb_device, descriptor, &info)) {
    return false;
  }

  OLA_INFO << "Nodle U1 serial: " << info.serial;

  // Check if it's a RP2040-based widget and if so, how many ins and outs it has
  int ret = 0;
  unsigned int ins = 1;   // Input universes
  unsigned int outs = 1;  // Output universes
  char variant = 'S';     // Variant: S = simple, R = RDM (not yet implemented)
  if (info.serial.find('RP2040_') != std::string::npos) {
    // Model format: ??Tx ??Rx S
    ret = sscanf(info.product.c_str(), "%uTx %uRx %c", &outs, &ins, &variant);
    if (ret == 3) {
      OLA_INFO << "It's a RP2040-based device with " << ins << " INs and " <<
        outs << " OUTs";
    } else {
      // Reset the values back to their default, just in case
      ins = 1;
      outs = 1;
      variant = 'S';
    }
  }

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
                                                 mode, ins, outs);
  } else {
    widget = new SynchronousDMXCProjectsNodleU1(m_adaptor, usb_device,
                                                m_plugin_adaptor, info.serial,
                                                mode, ins, outs);
  }
  return AddWidget(observer, widget);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
