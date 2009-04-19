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
 *
 * EspNetPort.cpp
 * The Esp-Net plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include "EspNetPort.h"
#include "EspNetDevice.h"
#include "common.h"

#include <lla/Logging.h>
#include <llad/Universe.h>

#include <string.h>

namespace lla {
namespace plugin {

#define min(a,b) a<b?a:b

EspNetPort::EspNetPort(EspNetDevice *parent, int id):
  Port(parent, id),
  m_len(DMX_UNIVERSE_SIZE),
  m_device(parent) {
    memset(m_buf, 0, DMX_UNIVERSE_SIZE);
}


bool EspNetPort::CanRead() const {
  // ports 0 to 4 are input
  return ( PortId()>=0 && PortId() < PORTS_PER_DEVICE);
}

bool EspNetPort::CanWrite() const {
  // ports 5 to 9 are output
  return ( PortId()>= PORTS_PER_DEVICE && PortId() < 2 * PORTS_PER_DEVICE);
}


/*
 * Write operation
 *
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 *
 */
int EspNetPort::WriteDMX(uint8_t *data, unsigned int length) {
  if (!CanWrite())
    return -1;

  if (espnet_send_dmx(m_device->EspnetNode(), GetUniverse()->UniverseId(), length, data)) {
    LLA_WARN << "espnet_send_dmx failed " << espnet_strerror();
    return -1;
  }
  return 0;
}

/*
 * Read operation
 *
 * @param   data  buffer to read data into
 * @param   length  length of data to read
 *
 * @return  the amount of data read
 */
int EspNetPort::ReadDMX(uint8_t *data, unsigned int length) {
  unsigned int len;

  if (!CanRead())
    return -1;

  len = min(m_len, length);
  memcpy(data, m_buf, len);
  return len;
}

/*
 * Update the data buffer for this port
 *
 */
int EspNetPort::UpdateBuffer(uint8_t *data, int length) {
  int len = min(DMX_UNIVERSE_SIZE, length);

  // we can't update if this isn't a input port
  if (!CanRead())
    return -1;

  memcpy(m_buf, data, len);
  DmxChanged();
  return 0;
}

} //plugin
} //lla
