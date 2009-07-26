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
 * OpenDmxDevice.cpp
 * The open dmx device
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "OpenDmxDevice.h"
#include "OpenDmxPort.h"

namespace ola {
namespace plugin {

using ola::Device;


/*
 * Create a new device
 *
 * @param owner
 * @param name
 * @param path to device
 */
OpenDmxDevice::OpenDmxDevice(AbstractPlugin *owner,
                             const string &name,
                             const string &path):
  Device(owner, name),
  m_path(path),
  m_enabled(false) {
}


/*
 * destroy this device
 */
OpenDmxDevice::~OpenDmxDevice() {
  if (m_enabled)
    Stop();
}


/*
 * Start this device
 *
 */
bool OpenDmxDevice::Start() {
  OpenDmxPort *port = new OpenDmxPort(this, 0, m_path);
  this->AddPort(port);
  m_enabled = true;
  return true;
}


/*
 * stop this device
 *
 */
bool OpenDmxDevice::Stop() {
  if (!m_enabled)
    return true;

  DeleteAllPorts();
  m_enabled = false;
  return true;
}

} //plugins
} //ola
