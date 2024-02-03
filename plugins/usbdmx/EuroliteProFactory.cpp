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
 * EuroliteProFactory.cpp
 * The WidgetFactory for EurolitePro widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/EuroliteProFactory.h"

#include <vector>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"
#include "ola/base/Flags.h"

DECLARE_bool(use_async_libusb);

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::LibUsbAdaptor;

// "Eurolite USB-DMX512-PRO"
const char EuroliteProFactory::EXPECTED_MANUFACTURER[] = "Eurolite";
const char EuroliteProFactory::EXPECTED_PRODUCT[] = "Eurolite DMX512 Pro";
const uint16_t EuroliteProFactory::PRODUCT_ID = 0xfa63;
const uint16_t EuroliteProFactory::VENDOR_ID = 0x04d8;

// "Eurolite USB-DMX512-PRO MK2" (successor device introduced in late 2016)
const char EuroliteProFactory::EXPECTED_MANUFACTURER_MK2[] = "FTDI";
const char EuroliteProFactory::EXPECTED_PRODUCT_MK2[] = "FT232R USB UART";
const uint16_t EuroliteProFactory::PRODUCT_ID_MK2 = 0x6001;
const uint16_t EuroliteProFactory::VENDOR_ID_MK2 = 0x0403;

const char EuroliteProFactory::ENABLE_EUROLITE_MK2_KEY[] =
    "enable_eurolite_mk2";
const char EuroliteProFactory::EUROLITE_MK2_SERIAL_KEY[] =
    "eurolite_mk2_serial";

EuroliteProFactory::EuroliteProFactory(ola::usb::LibUsbAdaptor *adaptor,
                                       Preferences *preferences)
  : BaseWidgetFactory<class EurolitePro>("EuroliteProFactory"),
    m_adaptor(adaptor),
    m_enable_eurolite_mk2(IsEuroliteMk2Enabled(preferences)) {
  const std::vector<std::string> serials =
      preferences->GetMultipleValue(EUROLITE_MK2_SERIAL_KEY);
  // A single empty string is considered the same as specifying
  // no serial numbers. This is useful as a default value.
  const bool has_default_value =
      serials.size() == 1 && serials[0].empty();
  if (!has_default_value) {
    for (std::vector<std::string>::const_iterator iter = serials.begin();
         iter != serials.end(); ++iter) {
      if (iter->empty()) {
        OLA_WARN << EUROLITE_MK2_SERIAL_KEY
                 << " requires a serial number, but it is empty.";
      } else if (STLContains(m_expected_eurolite_mk2_serials, *iter)) {
        OLA_WARN << EUROLITE_MK2_SERIAL_KEY << " lists serial "
                 << *iter << " more than once.";
      } else {
        m_expected_eurolite_mk2_serials.insert(*iter);
      }
    }
  }
}

bool EuroliteProFactory::IsEuroliteMk2Enabled(Preferences *preferences) {
  bool enabled;
  if (!StringToBool(preferences->GetValue(ENABLE_EUROLITE_MK2_KEY),
                    &enabled)) {
    enabled = false;
  }
  return enabled;
}

bool EuroliteProFactory::DeviceAdded(
    WidgetObserver *observer,
    libusb_device *usb_device,
    const struct libusb_device_descriptor &descriptor) {
  bool is_mk2 = false;
  LibUsbAdaptor::DeviceInformation info;

  // Eurolite USB-DMX512-PRO?
  if (descriptor.idVendor == VENDOR_ID && descriptor.idProduct == PRODUCT_ID) {
    OLA_INFO << "Found a new Eurolite USB-DMX512-PRO device";
    if (!m_adaptor->GetDeviceInfo(usb_device, descriptor, &info)) {
      return false;
    }

    if (!m_adaptor->CheckManufacturer(EXPECTED_MANUFACTURER, info)) {
      return false;
    }

    if (!m_adaptor->CheckProduct(EXPECTED_PRODUCT, info)) {
      return false;
    }

  // Eurolite USB-DMX512-PRO MK2?
  } else if (descriptor.idVendor == VENDOR_ID_MK2 &&
             descriptor.idProduct == PRODUCT_ID_MK2) {
    if (!m_adaptor->GetDeviceInfo(usb_device, descriptor, &info)) {
      return false;
    }

    const bool serial_matches =
        STLContains(m_expected_eurolite_mk2_serials, info.serial);

    if (m_enable_eurolite_mk2 || serial_matches) {
      if (serial_matches) {
        OLA_INFO << "Found a probable new Eurolite USB-DMX512-PRO MK2 device "
                 << "with matching serial " << info.serial;
      } else {
        OLA_INFO << "Found a probable new Eurolite USB-DMX512-PRO MK2 device "
                 << "with serial " << info.serial;
      }
      if (!m_adaptor->CheckManufacturer(EXPECTED_MANUFACTURER_MK2, info)) {
        return false;
      }

      if (!m_adaptor->CheckProduct(EXPECTED_PRODUCT_MK2, info)) {
        return false;
      }
      is_mk2 = true;
    } else {
      OLA_INFO << "Connected FTDI device with serial " << info.serial
               << " could be a Eurolite USB-DMX512-PRO MK2 but was "
               << "ignored, because "
               << ENABLE_EUROLITE_MK2_KEY << " was false and "
               << "its serial number was not listed specifically in "
               << EUROLITE_MK2_SERIAL_KEY;
      return false;
    }
  } else {
    // Something else
    return false;
  }

  // The original Eurolite doesn't have a serial number, so instead we use the
  // device & bus number. The MK2 does, so we use that where available.
  // TODO(simon): check if this supports the SERIAL NUMBER label and use that
  // instead.

  std::ostringstream serial_str;
  if (is_mk2 && !info.serial.empty()) {
    serial_str << info.serial;
  } else {
    // Original, there is no Serialnumber--> work around: bus+device number
    int bus_number = libusb_get_bus_number(usb_device);
    int device_address = libusb_get_device_address(usb_device);

    serial_str << bus_number << "-" << device_address;
  }

  EurolitePro *widget = NULL;
  if (FLAGS_use_async_libusb) {
    widget = new AsynchronousEurolitePro(m_adaptor, usb_device,
                                         serial_str.str(),
                                         is_mk2);
  } else {
    widget = new SynchronousEurolitePro(m_adaptor, usb_device,
                                        serial_str.str(),
                                        is_mk2);
  }
  return AddWidget(observer, widget);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
