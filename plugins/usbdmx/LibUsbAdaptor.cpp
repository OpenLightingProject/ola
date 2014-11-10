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
 * LibUsbAdaptor.h
 * The wrapper around libusb that abstracts syncronous vs asyncronous calls.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/LibUsbAdaptor.h"

#include <ola/Logging.h>
#include <libusb.h>

namespace ola {
namespace plugin {
namespace usbdmx {

void LibUsbAdaptor::SetDebug(libusb_context *context) {
  OLA_DEBUG << "libusb debug level set to " << m_debug_level;
  libusb_set_debug(context, m_debug_level);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
