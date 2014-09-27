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
 * KarateDevice.cpp
 * The KarateLight device
 * Copyright (C) 2005 Simon Newton
 */

#include <string>
#include <sstream>

#include "plugins/karate/KarateDevice.h"
#include "plugins/karate/KaratePort.h"

namespace ola {
namespace plugin {
namespace karate {

using ola::Device;
using std::string;

/**
 * @brief Create a new device
 */
KarateDevice::KarateDevice(AbstractPlugin *owner,
                           const string &name,
                           const string &path,
                           unsigned int device_id)
    : Device(owner, name),
      m_path(path) {
  std::ostringstream str;
  str << device_id;
  m_device_id = str.str();
}

/**
 * @brief Start this device
 */
bool KarateDevice::StartHook() {
  AddPort(new KarateOutputPort(this, 0, m_path));
  return true;
}
}  // namespace karate
}  // namespace plugin
}  // namespace ola
