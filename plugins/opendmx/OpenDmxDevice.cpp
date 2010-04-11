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
#include <string>

#include "plugins/opendmx/OpenDmxDevice.h"
#include "plugins/opendmx/OpenDmxPort.h"

namespace ola {
namespace plugin {
namespace opendmx {

using ola::Device;


/*
 * Create a new device
 * @param owner
 * @param name
 * @param path to device
 */
OpenDmxDevice::OpenDmxDevice(AbstractPlugin *owner,
                             const string &name,
                             const string &path):
  Device(owner, name),
  m_path(path) {
}


/*
 * Start this device
 */
bool OpenDmxDevice::StartHook() {
  AddPort(new OpenDmxOutputPort(this, 0, m_path));
  return true;
}
}  // opendmx
}  // plugins
}  // ola
