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
 * ArtnetPort.h
 * The ArtNet plugin for ola
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef PLUGINS_ARTNET_ARTNETPORT_H_
#define PLUGINS_ARTNET_ARTNETPORT_H_

#include <artnet/artnet.h>
#include <string>
#include "olad/Port.h"
#include "plugins/artnet/ArtNetDevice.h"

namespace ola {
namespace plugin {
namespace artnet {

class ArtNetPortHelper {
  public:
    ArtNetPortHelper(artnet_node node, bool is_output)
        : m_is_output(is_output),
          m_node(node) {}

    artnet_node GetNode() const { return m_node; }
    void PostSetUniverse(Universe *new_universe, unsigned int port_id);
    string Description(const Universe *universe,
                       unsigned int port_id) const;

  private:
    bool m_is_output;
    artnet_node m_node;
};


class ArtNetInputPort: public BasicInputPort {
  public:
    ArtNetInputPort(ArtNetDevice *parent,
                    unsigned int port_id,
                    const TimeStamp *wake_time,
                    artnet_node node)
        : BasicInputPort(parent, port_id, wake_time),
          m_helper(node, false) {}

    const DmxBuffer &ReadDMX() const;

    void PostSetUniverse(Universe *old_universe, Universe *new_universe) {
      (void) old_universe;
      m_helper.PostSetUniverse(new_universe, PortId());
    }

    string Description() const {
      return m_helper.Description(GetUniverse(), PortId());
    }

  private:
    mutable DmxBuffer m_buffer;
    ArtNetPortHelper m_helper;
};


class ArtNetOutputPort: public BasicOutputPort {
  public:
    ArtNetOutputPort(ArtNetDevice *device,
                     unsigned int port_id,
                     artnet_node node)
        : BasicOutputPort(device, port_id),
          m_helper(node, true) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

    void PostSetUniverse(Universe *old_universe, Universe *new_universe) {
      (void) old_universe;
      m_helper.PostSetUniverse(new_universe, PortId());
    }

    string Description() const {
      return m_helper.Description(GetUniverse(), PortId());
    }

  private:
    ArtNetPortHelper m_helper;
};
}  // artnet
}  // plugin
}  // ola
#endif  // PLUGINS_ARTNET_ARTNETPORT_H_
