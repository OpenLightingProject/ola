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
 * SunliteDevice.cpp
 * The Sunlite USBDMX2 device.
 * Copyright (C) 2010 Simon Newton
 */

#include "plugins/usbdmx/SunliteDevice.h"

#include "plugins/usbdmx/SunliteOutputPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

SunliteDevice::SunliteDevice(ola::AbstractPlugin *owner,
                             SunliteWidgetInterface *widget)
    : Device(owner, "Sunlite USBDMX2 Device"),
      m_port(new SunliteOutputPort(this, 0, widget)) {
}

bool SunliteDevice::StartHook() {
  AddPort(m_port.release());
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
