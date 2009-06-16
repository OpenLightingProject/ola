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
 * pathportport.cpp
 * The Pathport plugin for lla
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <string.h>

#include <lla/Logging.h>
#include <llad/universe.h>

#include "PathportPort.h"
#include "PathportDevice.h"
#include "PathportCommon.h"

namespace lla {
namespace plugin {

PathportPort::PathportPort(PathportDevice *parent, int id) :
  Port(parent, id),
  m_buf(NULL),
  m_len(DMX_UNIVERSE_SIZE) {
}


PathportPort::~PathportPort() {
  if (CanRead())
    free(m_buf);
}


int PathportPort::CanRead() const {
  return ( PortId() >=0 && PortId() < PORTS_PER_DEVICE / 2);
}


bool PathportPort::CanWrite() const {
  return ( PortId() >= PORTS_PER_DEVICE / 2 && PortId() < PORTS_PER_DEVICE);
}


/*
 * Write operation
 *
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 *
 */
int PathportPort::WriteDMX(uint8_t *data, unsigned int length) {
  PathportDevice *dev = (PathportDevice*) GetDevice();
  Universe *uni = GetUniverse();

  if (!CanWrite())
    return -1;

  if (!uni)
    return 0;

  if (pathport_send_dmx(dev->PathportNode(), uni->UniverseId(), length, data)) {
    LLA_WARN << "pathport_send_dmx failed " << pathport_strerror();
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
int PathportPort::ReadDMX(uint8_t *data, unsigned int length) {
  unsigned int len;

  if (!CanRead())
    return -1;

  len = m_len < length ? m_len : length;
  memcpy(data, m_buf, len);
  return len;
}


/*
 * Update the data buffer for this port
 *
 */
int PathportPort::UpdateBuffer(const uint8_t *data, int length) {
  int len = DMX_UNIVERSE_SIZE < length ? DMX_UNIVERSE_SIZE : length;

  // we can't update if this isn't a input port
  if (!CanRead())
    return -1;

  if (m_buf == NULL) {
    m_buf = (uint8_t*) malloc(m_len * sizeof(uint8_t));

    if (m_buf == NULL) {
      LLA_WARN << "malloc failed";
      return -1;
    } else
      memset(m_buf, 0x00, m_len);
  }

  memcpy(m_buf, data, len);

  DmxChanged();
  return 0;
}


/*
 * We override the set universe method to register our interest in
 * pathport universes
 */
int PathportPort::SetUniverse(Universe *uni) {
  pathport_node node = GetDevice()->PathportNode();

  Universe *old = GetUniverse();

  Port::SetUniverse(uni);

  if (CanRead()) {
    // Unregister our interest in this universe
    if (old) {
      pathport_unregister_uni(node, old->UniverseId());
      GetDevice()->port_map(old, NULL);
    }

    if (uni) {
      GetDevice()->port_map(uni, this);
      pathport_register_uni(node, uni->UniverseId());
    }
  }
  return 0;
}

} //plugin
} //lla
