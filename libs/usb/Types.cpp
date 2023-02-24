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
 * Types.cpp
 * Types used with the USB subsystem.
 * Copyright (C) 2015 Simon Newton
 */

#include "libs/usb/Types.h"
#include <ostream>

#include "ola/strings/Format.h"

namespace ola {
namespace usb {

using std::ostream;

USBDeviceID::USBDeviceID(uint8_t bus_number, uint8_t device_address)
    : bus_number(bus_number),
      device_address(device_address) {
}

bool USBDeviceID::operator<(const USBDeviceID &id) const {
  if (bus_number < id.bus_number) {
    return true;
  } else if (bus_number == id.bus_number) {
    return device_address < id.device_address;
  }
  return false;
}

ostream& operator<<(ostream& os, const USBDeviceID &id) {
  return os << ola::strings::IntToString(id.bus_number) << ":"
            << ola::strings::IntToString(id.device_address);
}
}  // namespace usb
}  // namespace ola
