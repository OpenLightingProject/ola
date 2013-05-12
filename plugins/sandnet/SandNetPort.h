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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * SandNetPort.h
 * The SandNet plugin for ola
 * Copyright (C) 2005-2006  Simon Newton
 */

#ifndef PLUGINS_SANDNET_SANDNETPORT_H_
#define PLUGINS_SANDNET_SANDNETPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/sandnet/SandNetDevice.h"

namespace ola {
namespace plugin {
namespace sandnet {

class SandNetPortHelper {
  public:
    SandNetPortHelper() {}
    bool PreSetUniverse(Universe *old_universe, Universe *new_universe);
    string Description(const Universe *universe) const;
    uint8_t SandnetGroup(const Universe* universe) const;
    uint8_t SandnetUniverse(const Universe *universe) const;
};


class SandNetInputPort: public BasicInputPort {
  public:
    SandNetInputPort(SandNetDevice *parent,
                     unsigned int id,
                     class PluginAdaptor *plugin_adaptor,
                     SandNetNode *node):
      BasicInputPort(parent, id, plugin_adaptor),
      m_node(node) {}
    ~SandNetInputPort() {}

    string Description() const { return m_helper.Description(GetUniverse()); }
    const DmxBuffer &ReadDMX() const { return m_buffer; }
    bool PreSetUniverse(Universe *old_universe, Universe *new_universe) {
      return m_helper.PreSetUniverse(old_universe, new_universe);
    }
    void PostSetUniverse(Universe *old_universe, Universe *new_universe);

  private:
    SandNetPortHelper m_helper;
    SandNetNode *m_node;
    DmxBuffer m_buffer;
};


class SandNetOutputPort: public BasicOutputPort {
  public:
    SandNetOutputPort(SandNetDevice *parent,
                      unsigned int id,
                      SandNetNode *node):
      BasicOutputPort(parent, id),
      m_node(node) {}
    ~SandNetOutputPort() {}

    string Description() const { return m_helper.Description(GetUniverse()); }
    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    bool PreSetUniverse(Universe *old_universe, Universe *new_universe) {
      return m_helper.PreSetUniverse(old_universe, new_universe);
    }
    void PostSetUniverse(Universe *old_universe, Universe *new_universe);

  private:
    SandNetPortHelper m_helper;
    SandNetNode *m_node;
};
}  // namespace sandnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SANDNET_SANDNETPORT_H_
