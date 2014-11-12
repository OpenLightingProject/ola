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
 * AnymaDevice.cpp
 * The Anyma uDMX device.
 * Copyright (C) 2010 Simon Newton
 */

#include "plugins/usbdmx/AnymaDevice.h"

#include "plugins/usbdmx/AnymaWidget.h"
#include "plugins/usbdmx/GenericOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

AnymaDevice::AnymaDevice(ola::AbstractPlugin *owner,
                         AnymaWidget *widget)
    : Device(owner, "Anyma USB Device"),
      m_device_id("anyma-" + widget->SerialNumber()),
      m_port(new GenericOutputPort(this, 0, widget)) {
}

bool AnymaDevice::StartHook() {
  AddPort(m_port.release());
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
