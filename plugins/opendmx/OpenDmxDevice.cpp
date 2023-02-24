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
 * OpenDmxDevice.cpp
 * The Open DMX device
 * Copyright (C) 2005 Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>

#include "plugins/opendmx/OpenDmxDevice.h"
#include "plugins/opendmx/OpenDmxPort.h"

namespace ola {
namespace plugin {
namespace opendmx {

using ola::Device;
using std::string;


/*
 * Create a new device
 * @param owner
 * @param name
 * @param path to device
 */
OpenDmxDevice::OpenDmxDevice(AbstractPlugin *owner,
                             const string &name,
                             const string &path,
                             unsigned int device_id)
    : Device(owner, name),
      m_path(path) {
  std::ostringstream str;
  str << device_id;
  m_device_id = str.str();
}



/*
 * Start this device
 */
bool OpenDmxDevice::StartHook() {
  AddPort(new OpenDmxOutputPort(this, 0, m_path));
  return true;
}
}  // namespace opendmx
}  // namespace plugin
}  // namespace ola
