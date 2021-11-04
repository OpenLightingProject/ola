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
 * USBDMXComFactory.cpp
 * The WidgetFactory for USBDMXCom widgets.
 * Copyright (C) 2021 Peter Newman
 */

#include "plugins/usbdmx/USBDMXComFactory.h"

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"
#include "ola/base/Flags.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::LibUsbAdaptor;

// "USBDMX.com"
const char USBDMXComFactory::EXPECTED_MANUFACTURER[] = "USBDMX.COM";
const char USBDMXComFactory::EXPECTED_PRODUCT[] = "DMX Adapter";
const uint16_t USBDMXComFactory::PRODUCT_ID = 0x6001;
const uint16_t USBDMXComFactory::VENDOR_ID = 0x0403;

const char USBDMXComFactory::ENABLE_USBDMXCOM_KEY[] =
    "enable_usbdmxcom";

USBDMXComFactory::USBDMXComFactory(ola::usb::LibUsbAdaptor *adaptor,
                                   Preferences *preferences)
  : BaseWidgetFactory<class USBDMXCom>("USBDMXComFactory"),
    m_adaptor(adaptor),
    m_enable_usbdmxcom(IsUSBDMXComEnabled(preferences)) {
}

bool USBDMXComFactory::IsUSBDMXComEnabled(Preferences *preferences) {
  bool enabled;
  if (!StringToBool(preferences->GetValue(ENABLE_USBDMXCOM_KEY),
                    &enabled)) {
    enabled = false;
  }
  return enabled;
}

bool USBDMXComFactory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  // USBDMX.com?
  if (descriptor.idVendor == VENDOR_ID && descriptor.idProduct == PRODUCT_ID) {
    if (m_enable_usbdmxcom) {
      OLA_INFO << "Found a possible new USBDMX.com device";
      LibUsbAdaptor::DeviceInformation info;
      if (!m_adaptor->GetDeviceInfo(usb_device, descriptor, &info)) {
        return false;
      }

      if (!m_adaptor->CheckManufacturer(EXPECTED_MANUFACTURER, info)) {
        return false;
      }

      if (!m_adaptor->CheckProduct(EXPECTED_PRODUCT, info)) {
        return false;
      }
    } else {
      OLA_INFO << "Connected FTDI device could be a USBDMX.com but was "
               << "ignored, because " << ENABLE_USBDMXCOM_KEY << " was false.";
      return false;
    }
  } else {
    // Something else
    return false;
  }

  // TODO(Someone): Is this true for the USBDMX.com?
  // The USBDMX.com doesn't have a serial number, so instead we use the device &
  // bus number.
  // TODO(Someone): check if this supports the SERIAL NUMBER label and use that
  // instead.

  // There is no Serialnumber--> work around: bus+device number
  int bus_number = libusb_get_bus_number(usb_device);
  int device_address = libusb_get_device_address(usb_device);

  std::ostringstream serial_str;
  serial_str << bus_number << "-" << device_address;

  USBDMXCom *widget = NULL;
  if (FLAGS_use_async_libusb) {
    widget = new AsynchronousUSBDMXCom(m_adaptor, usb_device,
                                       serial_str.str());
  } else {
    widget = new SynchronousUSBDMXCom(m_adaptor, usb_device,
                                      serial_str.str());
  }
  return AddWidget(observer, widget);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
