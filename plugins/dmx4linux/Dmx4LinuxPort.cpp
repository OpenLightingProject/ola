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

namespace lla {
namespace plugin {

Dmx4LinuxPort::Dmx4LinuxPort(Dmx4LinuxDevice *parent,
                             int id,
                             int dmx_universe,
                             bool in,
                             bool out):
  Port(parent, id),
  m_in(in),
  m_out(out),
  m_dmx_universe(dmx_universe),
  m_device(parent) {
}


/*
 * Write operation
 *
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 *
 * @return   0 on success, non 0 on failure
 */
int Dmx4LinuxPort::WriteDMX(uint8_t *data, unsigned int length) {
  if (!CanWrite())
    return -1;

  // send to device
  return m_device->SendDmx(m_dmx_universe, data, length);

}

/*
 * Read operation
 *
 * @param   data  buffer to read data into
 * @param   length  length of data to read
 *
 * @return  the amount of data read
 */
int Dmx4LinuxPort::ReadDMX(uint8_t *data, unsigned int length) {
  data = NULL;
  length = 0;
  return -1;
}

} //plugin
} //lla
