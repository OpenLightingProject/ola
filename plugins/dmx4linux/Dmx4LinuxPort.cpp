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
 * Dmx4LinuxPort.cpp
 * The DMX 4 Linux plugin
 * Copyright (C) 2006-2008 Simon Newton
 */

#include <string.h>
#include "Dmx4LinuxPort.h"
#include "Dmx4LinuxDevice.h"

namespace ola {
namespace plugin {


/*
 * Constructor
 * We only have 1 port per device so the id is always 0
 */
Dmx4LinuxPort::Dmx4LinuxPort(Dmx4LinuxDevice *parent,
                             int dmx_universe,
                             bool in,
                             bool out):
  Port<Dmx4LinuxDevice>(parent, 0),
  m_in(in),
  m_out(out),
  m_dmx_universe(dmx_universe) {
}


/*
 * Write operation
 * @param buffer the DmxBuffer to write
 * @return true on success, false on failure
 */
bool Dmx4LinuxPort::WriteDMX(const DmxBuffer &buffer) {
  if (!CanWrite())
    return false;

  return GetDevice()->SendDMX(m_dmx_universe, buffer);
}


/*
 * Read operation
 * @return a DmxBufer with the data
 */
const DmxBuffer &Dmx4LinuxPort::ReadDMX() const {
  return m_read_buffer;
}

} //plugin
} //ola
