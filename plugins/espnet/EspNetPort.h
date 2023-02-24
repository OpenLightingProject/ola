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
 * EspNetPort.h
 * The ESPNet plugin for ola
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_ESPNET_ESPNETPORT_H_
#define PLUGINS_ESPNET_ESPNETPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/espnet/EspNetDevice.h"

namespace ola {
namespace plugin {
namespace espnet {

class EspNetPortHelper {
 public:
    std::string Description(Universe *universe) const;
    uint8_t EspNetUniverseId(Universe *universe) const;
};


class EspNetInputPort: public BasicInputPort {
 public:
    EspNetInputPort(EspNetDevice *parent, unsigned int id,
                    class PluginAdaptor *plugin_adaptor,
                    EspNetNode *node)
        : BasicInputPort(parent, id, plugin_adaptor),
          m_helper(),
          m_node(node) {}
    ~EspNetInputPort() {}

    std::string Description() const {
      return m_helper.Description(GetUniverse());
    }
    void PostSetUniverse(Universe *old_universe, Universe *new_universe);
    const DmxBuffer &ReadDMX() const { return m_buffer; }

 private:
    EspNetPortHelper m_helper;
    EspNetNode *m_node;
    DmxBuffer m_buffer;
};


class EspNetOutputPort: public BasicOutputPort {
 public:
    EspNetOutputPort(EspNetDevice *parent, unsigned int id, EspNetNode *node)
        : BasicOutputPort(parent, id),
          m_helper(),
          m_node(node) {}
    ~EspNetOutputPort() {}

    std::string Description() const {
      return m_helper.Description(GetUniverse());
    }
    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

 private:
    EspNetPortHelper m_helper;
    EspNetNode *m_node;
};
}  // namespace espnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_ESPNET_ESPNETPORT_H_
