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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Dmx4LinuxDevice.cpp
 * Dmx4Linux device
 * Copyright (C) 2006-2009 Simon Newton
 *
 */
#include <string>
#include "plugins/dmx4linux/Dmx4LinuxPlugin.h"
#include "plugins/dmx4linux/Dmx4LinuxDevice.h"

namespace ola {
namespace plugin {
namespace dmx4linux {

using ola::Device;

/*
 * Create a new device
 * @param owner the plugin that owns this device
 * @param name  the device name
 * @param device_id the device id
 */
Dmx4LinuxDevice::Dmx4LinuxDevice(Dmx4LinuxPlugin *owner,
                                 const string &name,
                                 const string &device_id):
  Device(owner, name),
  m_device_id(device_id) {
}
}  // dmx4linux
}  // plugin
}  // ola
