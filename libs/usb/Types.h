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
 * Types.h
 * Types used with the USB subsystem.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef LIBS_USB_TYPES_H_
#define LIBS_USB_TYPES_H_

#include <stdint.h>
#include <ostream>

namespace ola {
namespace usb {

class USBDeviceID {
 public:
  USBDeviceID(uint8_t bus_number, uint8_t device_address);

  const uint8_t bus_number;
  const uint8_t device_address;

  bool operator<(const USBDeviceID &id) const;

  friend std::ostream& operator<<(std::ostream& os, const USBDeviceID &id);
};
}  // namespace usb
}  // namespace ola

#endif  // LIBS_USB_TYPES_H_
