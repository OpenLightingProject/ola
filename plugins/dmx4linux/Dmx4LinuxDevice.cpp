/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Dmx4LinuxDevice.cpp
 * Dmx4Linux device
 * Copyright (C) 2006-2009 Simon Newton
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <llad/Preferences.h>
#include <llad/Universe.h>

#include "Dmx4LinuxPlugin.h"
#include "Dmx4LinuxDevice.h"
#include "Dmx4LinuxPort.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

namespace lla {
namespace plugin {

using lla::Device;
using lla::Universe;

/*
 * Create a new device
 *
 * @param owner  the plugin that owns this device
 * @param name  the device name
 */
Dmx4LinuxDevice::Dmx4LinuxDevice(Dmx4LinuxPlugin *owner,
                                 const string &name,
                                 const string &device_id):
  Device(owner, name),
  m_plugin(owner),
  m_enabled(false),
  m_device_id(device_id) {
}


/*
 * Destroy this device
 */
Dmx4LinuxDevice::~Dmx4LinuxDevice() {
  if (m_enabled)
    Stop();
}


/*
 * Start this device
 */
bool Dmx4LinuxDevice::Start() {
  m_enabled = true;
  return true;
}


/*
 * Stop this device
 */
bool Dmx4LinuxDevice::Stop() {
  if (!m_enabled)
    return true;

  DeleteAllPorts();
  m_enabled = false;
  return true;
}


/*
 * Send the dmx out the widget
 * @return true on success, false on failure
 */
bool Dmx4LinuxDevice::SendDMX(int d4l_uni, const DmxBuffer &buffer) const {
  return m_plugin->SendDMX(d4l_uni, buffer);
}

} //plugin
} //lla
