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

#include <utility>
#include <ostream>

#include "ola/strings/Format.h"

namespace ola {
namespace usb {

/**
 * @brief Represents a USB device on the bus
 * @tparam bus_address The bus number the device is connected to.
 * @tparam device_address The address of the device on the bus.
 */
typedef std::pair<uint8_t, uint8_t> USBDeviceID;


/**
 * @brief Format a USBDeviceID to a stream
 */
inline std::ostream& operator<<(std::ostream &out, const USBDeviceID &id) {
  return out << ola::strings::IntToString(id.first) << ":"
             << ola::strings::IntToString(id.second);
}
}  // namespace usb
}  // namespace ola

#endif  // LIBS_USB_TYPES_H_
