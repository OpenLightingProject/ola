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
 * ArtNetPort.cpp
 * The ArtNet plugin for ola
 * Copyright (C) 2005 - 2009 Simon Newton
 */
#include <string.h>

#include <ola/Logging.h>
#include <olad/Universe.h>

#include "ArtNetPort.h"
#include "ArtNetDevice.h"

namespace ola {
namespace plugin {

bool ArtNetPort::IsOutput() const {
  // even ports are input
  return (PortId() % 2);
}

/*
 * Write operation
 * @param data pointer to the dmx data
 * @param length the length of the data
 * @return true if the write succeeded, false otherwise
 */
bool ArtNetPort::WriteDMX(const DmxBuffer &buffer) {
  ArtNetDevice *dev = GetDevice();
  if (!IsOutput())
    return false;

  if (artnet_send_dmx(dev->GetArtnetNode(), this->PortId() / 2,
                      buffer.Size(), buffer.GetRaw())) {
    OLA_WARN << "artnet_send_dmx failed " << artnet_strerror();
    return false;
  }
  return true;
}


/*
 * Read operation, attempting to read from a write-only port is undefined.
 * @return A DmxBuffer with the data.
 */
const DmxBuffer &ArtNetPort::ReadDMX() const {
  if (IsOutput())
    return m_buffer;

  int length;
  ArtNetDevice *dev = GetDevice();
  uint8_t *dmx_data = artnet_read_dmx(dev->GetArtnetNode(), PortId() / 2,
                                      &length);

  if(!dmx_data) {
    OLA_WARN << "artnet_read_dmx failed " << artnet_strerror();
    m_buffer.Reset();
    return m_buffer;
  }
  m_buffer.Set(dmx_data, length);
  return m_buffer;
}


/*
 * We override the set universe method to reprogram our port.
 */
bool ArtNetPort::SetUniverse(Universe *uni) {
  ArtNetDevice *dev = GetDevice();
  artnet_node node = dev->GetArtnetNode();

  // base method
  Port<ArtNetDevice>::SetUniverse(uni);

  // this is a bit of a hack but currently in libartnet there is no
  // way to disable a port once it's been enabled.
  if (!uni)
    return true;

  int artnet_port_id = uni->UniverseId() % ARTNET_MAX_PORTS;

  // carefull here, a port that we read from (input) is actually
  // an ArtNet output port
  if (!IsOutput()) {
    // input port
    if (artnet_set_port_type(node, PortId() / 2, ARTNET_ENABLE_OUTPUT,
                             ARTNET_PORT_DMX)) {
      OLA_WARN << "artnet_set_port_type failed " << artnet_strerror();
      return false;
    }

    if (artnet_set_port_addr(node, PortId() / 2, ARTNET_OUTPUT_PORT,
                             artnet_port_id)) {
      OLA_WARN << "artnet_set_port_addr failed " << artnet_strerror();
      return false;
    }

  } else if (IsOutput()) {
    if (artnet_set_port_type(node, PortId() / 2,
                             ARTNET_ENABLE_INPUT, ARTNET_PORT_DMX)) {
      OLA_WARN << "artnet_set_port_type failed " << artnet_strerror();
      return false;
    }
    if (artnet_set_port_addr(node, PortId() / 2,
                             ARTNET_INPUT_PORT, artnet_port_id)) {
      OLA_WARN << "artnet_set_port_addr failed " << artnet_strerror();
      return false;
    }
  }
  return true;
}

/*
 * Return the port description
 */
string ArtNetPort::Description() const {
  if (!GetUniverse())
    return "";

  ArtNetDevice *dev = GetDevice();
  artnet_node node = dev->GetArtnetNode();
  int universe_address = artnet_get_universe_addr(
      node,
      PortId() / 2,
      IsOutput() ? ARTNET_INPUT_PORT : ARTNET_OUTPUT_PORT);
  std::stringstream str;
  str << "ArtNet Universe " << universe_address;
  return str.str();
}


} //plugin
} //ola
