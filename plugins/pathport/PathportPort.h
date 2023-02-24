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
 * PathportPort.h
 * The Pathport plugin for ola
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_PATHPORT_PATHPORTPORT_H_
#define PLUGINS_PATHPORT_PATHPORTPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/pathport/PathportDevice.h"

namespace ola {
namespace plugin {
namespace pathport {

class PathportPortHelper {
 public:
    PathportPortHelper() {}
    std::string Description(const Universe *universe) const;
    bool PreSetUniverse(Universe *new_universe);
};


class PathportInputPort: public BasicInputPort {
 public:
    PathportInputPort(PathportDevice *parent,
                     unsigned int id,
                     class PluginAdaptor *plugin_adaptor,
                     PathportNode *node):
      BasicInputPort(parent, id, plugin_adaptor),
      m_node(node) {}
    ~PathportInputPort() {}

    std::string Description() const {
      return m_helper.Description(GetUniverse());
    }
    const DmxBuffer &ReadDMX() const { return m_buffer; }
    bool PreSetUniverse(OLA_UNUSED Universe *old_universe,
                        Universe *new_universe) {
      return m_helper.PreSetUniverse(new_universe);
    }

    void PostSetUniverse(Universe *old_universe, Universe *new_universe);

 private:
    PathportPortHelper m_helper;
    PathportNode *m_node;
    DmxBuffer m_buffer;
};


class PathportOutputPort: public BasicOutputPort {
 public:
    PathportOutputPort(PathportDevice *parent,
                      unsigned int id,
                      PathportNode *node):
      BasicOutputPort(parent, id),
      m_node(node) {}
    ~PathportOutputPort() {}

    std::string Description() const {
      return m_helper.Description(GetUniverse());
    }
    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    bool PreSetUniverse(OLA_UNUSED Universe *old_universe,
                        Universe *new_universe) {
      return m_helper.PreSetUniverse(new_universe);
    }

 private:
    PathportPortHelper m_helper;
    PathportNode *m_node;
};
}  // namespace pathport
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_PATHPORT_PATHPORTPORT_H_
