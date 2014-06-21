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
 * ShowNetPort.h
 * The ShowNet plugin for ola
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_SHOWNET_SHOWNETPORT_H_
#define PLUGINS_SHOWNET_SHOWNETPORT_H_

#include <string>
#include "olad/Port.h"
#include "plugins/shownet/ShowNetDevice.h"
#include "plugins/shownet/ShowNetNode.h"

namespace ola {
namespace plugin {
namespace shownet {

class ShowNetInputPort: public BasicInputPort {
 public:
  ShowNetInputPort(ShowNetDevice *parent,
                   unsigned int id,
                   class PluginAdaptor *plugin_adaptor,
                   ShowNetNode *node):
    BasicInputPort(parent, id, plugin_adaptor),
    m_node(node) {}
  ~ShowNetInputPort() {}

  std::string Description() const;
  const ola::DmxBuffer &ReadDMX() const { return m_buffer; }
  bool PreSetUniverse(Universe *old_universe, Universe *new_universe);
  void PostSetUniverse(Universe *old_universe, Universe *new_universe);

 private:
  DmxBuffer m_buffer;
  ShowNetNode *m_node;
};


class ShowNetOutputPort: public BasicOutputPort {
 public:
  ShowNetOutputPort(ShowNetDevice *parent,
                    unsigned int id,
                    ShowNetNode *node):
    BasicOutputPort(parent, id),
    m_node(node) {}
  ~ShowNetOutputPort() {}

  bool PreSetUniverse(Universe *old_universe, Universe *new_universe);
  std::string Description() const;
  bool WriteDMX(const ola::DmxBuffer &buffer, uint8_t priority);

 private:
  ShowNetNode *m_node;
};
}  // namespace shownet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SHOWNET_SHOWNETPORT_H_
