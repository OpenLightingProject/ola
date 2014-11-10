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
 * LibUsbUtils.cpp
 * libusb Util functions.
 * Copyright (C) 2014 Peter Newman
 */

#include <string>

#include "ola/Logging.h"
#include "plugins/usbdmx/LibUsbUtils.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

/**
 * Return a string descriptor.
 * @param usb_handle the usb handle to the device
 * @param desc_index the index of the descriptor
 * @param data where to store the output string
 * @returns true if we got the value, false otherwise
 */
bool GetDescriptorString(libusb_device_handle *usb_handle,
                         uint8_t desc_index,
                         string *data) {
  enum { buffer_size = 32 };  // static arrays FTW!
  unsigned char buffer[buffer_size];
  int r = libusb_get_string_descriptor_ascii(usb_handle,
                                             desc_index,
                                             buffer,
                                             buffer_size);

  if (r <= 0) {
    OLA_INFO << "libusb_get_string_descriptor_ascii returned " << r;
    return false;
  }
  data->assign(reinterpret_cast<char*>(buffer));
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
