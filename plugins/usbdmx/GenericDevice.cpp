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
 * GenericDevice.cpp
 * A Generic device that creates a single port.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/GenericDevice.h"

#include <string>
#include "plugins/usbdmx/Widget.h"
#include "plugins/usbdmx/GenericOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

GenericDevice::GenericDevice(ola::AbstractPlugin *owner,
                             WidgetInterface *widget,
                             const std::string &device_name,
                             const std::string &device_id)
    : Device(owner, device_name),
      m_device_id(device_id),
      m_port(new GenericOutputPort(this, 0, widget)) {
}

bool GenericDevice::StartHook() {
  AddPort(m_port.release());
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
