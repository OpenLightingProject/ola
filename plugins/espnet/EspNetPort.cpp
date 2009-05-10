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
 * EspNetPort.cpp
 * The Esp-Net plugin for lla
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include <lla/Logging.h>
#include <llad/Universe.h>

#include "EspNetPort.h"
#include "EspNetDevice.h"
#include "common.h"


namespace lla {
namespace plugin {

EspNetPort::EspNetPort(EspNetDevice *parent, unsigned int id):
  Port(parent, id),
  m_device(parent) {
}


bool EspNetPort::CanRead() const {
  // ports 0 to 4 are input
  return PortId() < PORTS_PER_DEVICE;
}

bool EspNetPort::CanWrite() const {
  // ports 5 to 9 are output
  return (PortId()>= PORTS_PER_DEVICE && PortId() < 2 * PORTS_PER_DEVICE);
}


/*
 * Write operation
 *
 */
bool EspNetPort::WriteDMX(const DmxBuffer &buffer) {
  if (!CanWrite())
    return false;

  if (espnet_send_dmx(m_device->EspnetNode(), GetUniverse()->UniverseId(),
        buffer.Size(), buffer.GetRaw())) {
    LLA_WARN << "espnet_send_dmx failed " << espnet_strerror();
    return false;
  }
  return true;
}


/*
 * Read operation
 * @return The DmxBuffer with the new data
 */
const DmxBuffer &EspNetPort::ReadDMX() const {
  return m_buffer;
}


/*
 * Update the data buffer for this port
 */
bool EspNetPort::UpdateBuffer(uint8_t *data, int length) {
  // we can't update if this isn't a input port
  if (!CanRead())
    return false;

  m_buffer.Set(data, length);
  DmxChanged();
  return true;
}

} //plugin
} //lla
