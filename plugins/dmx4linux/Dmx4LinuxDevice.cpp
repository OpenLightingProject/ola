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
 * dmx4linuxdevice.cpp
 * Dmx4Linux device
 * Copyright (C) 2006-2008 Simon Newton
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
Dmx4LinuxDevice::Dmx4LinuxDevice(Dmx4LinuxPlugin *owner, const string &name) :
  Device(owner, name),
  m_plugin(owner),
  m_enabled(false) {
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
  return 0;
}


/*
 * Stop this device
 */
bool Dmx4LinuxDevice::Stop() {
  if (!m_enabled)
    return true;

  DeleteAllPorts();
  m_enabled = false;
  return 0;
}


/*
 * Send the dmx out the widget
 * called from the Dmx4LinuxPort
 *
 * @return   0 on success, non 0 on failure
 */
int Dmx4LinuxDevice::SendDmx(int d4l_uni, uint8_t *data, int len) {
  return m_plugin->SendDmx(d4l_uni, data, len);
}

} //plugin
} //lla
