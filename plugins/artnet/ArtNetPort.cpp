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
 * artnetport.cpp
 * The Art-Net plugin for lla
 * Copyright (C) 2005 - 2006 Simon Newton
 */
#include <string.h>

#include <lla/Logging.h>
#include <llad/Universe.h>

#include "ArtNetPort.h"
#include "ArtNetDevice.h"

namespace lla {
namespace plugin {

bool ArtNetPort::CanRead() const {
  // ports 0 to 3 are input
  return ( PortId() >= 0 && PortId() < ARTNET_MAX_PORTS);
}

bool ArtNetPort::CanWrite() const {
  // ports 4 to 7 are output
  return ( PortId() >= ARTNET_MAX_PORTS && PortId() < 2 * ARTNET_MAX_PORTS);
}


/*
 * Write operation
 *
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 *
 */
int ArtNetPort::WriteDMX(uint8_t *data, unsigned int length) {
  ArtNetDevice *dev = (ArtNetDevice*) GetDevice();

  if (!CanWrite())
    return -1;

  if (artnet_send_dmx(dev->GetArtnetNode() , this->PortId() % 4 , length, data)) {
    LLA_WARN << "artnet_send_dmx failed " << artnet_strerror();
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
int ArtNetPort::ReadDMX(uint8_t *data, unsigned int length) {
  uint8_t *dmx = NULL;
  int len;
  ArtNetDevice *dev = (ArtNetDevice*) GetDevice();

  if (!CanRead())
    return -1;

  dmx = artnet_read_dmx(dev->GetArtnetNode(), PortId(), &len);

  if(dmx == NULL) {
    LLA_WARN << "artnet_read_dmx failed " << artnet_strerror();
    return -1;
  }
  len = len < (int) length ? len : (int) length;

  memcpy(data, dmx, len);
  return len;
}


/*
 * We override the set universe method to reprogram our
 */
int ArtNetPort::SetUniverse(Universe *uni) {
  ArtNetDevice *dev = (ArtNetDevice*) GetDevice();
  artnet_node node = dev->GetArtnetNode();
  int id = PortId();
  int port_id;

  // base method
  Port::SetUniverse(uni);

  // this is a bit of a hack but currently in libartnet there is no
  // way to disable a port once it's been enabled.
  if(!uni)
    return 0;

  port_id = uni->UniverseId();
  port_id %= ARTNET_MAX_PORTS;

  // carefull here, a port that we read from (input) is actually
  // an ArtNet output port
  if (id >= 0 && id <= 3) {
    // input port
    if (artnet_set_port_type(node, id, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX)) {
      LLA_WARN << "artnet_set_port_type failed " << artnet_strerror();
      return -1;
    }

    if (artnet_set_port_addr(node, id, ARTNET_OUTPUT_PORT, port_id)) {
      LLA_WARN << "artnet_set_port_addr failed " << artnet_strerror();
      return -1;
    }

  } else if (id >= 4 && id <= 7) {
    if (artnet_set_port_type(node, id-4, ARTNET_ENABLE_INPUT, ARTNET_PORT_DMX)) {
      LLA_WARN << "artnet_set_port_type failed " << artnet_strerror();
      return -1;
    }
    if (artnet_set_port_addr(node, id-4, ARTNET_INPUT_PORT, port_id)) {
      LLA_WARN << "artnet_set_port_addr failed " << artnet_strerror();
      return -1;
    }
  }
  return 0;
}

} //plugin
} //lla
