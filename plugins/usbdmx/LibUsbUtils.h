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
 * LibUsbUtils.h
 * libusb Util functions.
 * Copyright (C) 2014 Peter Newman
 */

#ifndef PLUGINS_USBDMX_LIBUSBUTILS_H_
#define PLUGINS_USBDMX_LIBUSBUTILS_H_

#include <libusb.h>

#include <string>

namespace ola {
namespace plugin {
namespace usbdmx {

bool GetDescriptorString(libusb_device_handle *usb_handle,
                         uint8_t desc_index,
                         std::string *data);
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_LIBUSBUTILS_H_
