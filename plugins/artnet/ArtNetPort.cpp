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
#include <artnet/artnet.h>
#include <string.h>
#include <string>

#include "ola/Logging.h"
#include "olad/Universe.h"
#include "plugins/artnet/ArtNetDevice.h"
#include "plugins/artnet/ArtNetPort.h"

namespace ola {
namespace plugin {
namespace artnet {

/*
 * Reprogram our port.
 */
void ArtNetPortHelper::PostSetUniverse(Universe *new_universe,
                                       unsigned int port_id) {
  // this is a bit of a hack but currently in libartnet there is no
  // way to disable a port once it's been enabled.
  if (!new_universe)
    return;

  // carefull here, a port that we read from (input) is actually
  // an ArtNet output port
  int r = artnet_set_port_type(
      m_node,
      port_id,
      m_is_output ? ARTNET_ENABLE_INPUT : ARTNET_ENABLE_OUTPUT,
      ARTNET_PORT_DMX);
  if (r) {
    OLA_WARN << "artnet_set_port_type failed " << artnet_strerror();
    return;
  }

  r = artnet_set_port_addr(
      m_node,
      port_id,
      m_is_output ? ARTNET_INPUT_PORT : ARTNET_OUTPUT_PORT,
      new_universe->UniverseId() % 0x10);  // Artnet universes are from 0-15
  if (r) {
    OLA_WARN << "artnet_set_port_addr failed " << artnet_strerror();
    return;
  }
}


/*
 * Return the port description
 */
string ArtNetPortHelper::Description(const Universe *universe,
                                     unsigned int port_id) const {
  if (!universe)
    return "";

  int universe_address = artnet_get_universe_addr(
      m_node,
      port_id,
      m_is_output ? ARTNET_INPUT_PORT : ARTNET_OUTPUT_PORT);
  std::stringstream str;
  str << "ArtNet Universe " << universe_address;
  return str.str();
}


/*
 * Read operation.
 * @return A DmxBuffer with the data.
 */
const DmxBuffer &ArtNetInputPort::ReadDMX() const {
  if (PortId() >= ARTNET_MAX_PORTS) {
    OLA_WARN << "Invalid artnet port id " << PortId();
    return m_buffer;
  }

  int length;
  uint8_t *dmx_data = artnet_read_dmx(m_helper.GetNode(), PortId(), &length);

  if (!dmx_data) {
    OLA_WARN << "artnet_read_dmx failed " << artnet_strerror();
    m_buffer.Reset();
    return m_buffer;
  }
  m_buffer.Set(dmx_data, length);
  return m_buffer;
}


/*
 * Write operation
 * @param data pointer to the dmx data
 * @param length the length of the data
 * @return true if the write succeeded, false otherwise
 */
bool ArtNetOutputPort::WriteDMX(const DmxBuffer &buffer,
                                uint8_t priority) {
  if (PortId() >= ARTNET_MAX_PORTS) {
    OLA_WARN << "Invalid artnet port id " << PortId();
    return false;
  }

  if (artnet_send_dmx(m_helper.GetNode(), PortId(), buffer.Size(),
                      buffer.GetRaw())) {
    OLA_WARN << "artnet_send_dmx failed " << artnet_strerror();
    return false;
  }
  return true;
}
}  // artnet
}  // plugin
}  // ola
