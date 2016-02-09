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
 * E131Port.h
 * The E1.31 port for OLA
 * Copyright (C) 2007 Simon Newton
 */

#ifndef PLUGINS_E131_E131PORT_H_
#define PLUGINS_E131_E131PORT_H_

#include <string>
#include "olad/Port.h"
#include "plugins/e131/E131Device.h"
#include "libs/acn/E131Node.h"

namespace ola {
namespace plugin {
namespace e131 {

class E131PortHelper {
 public:
  bool PreSetUniverse(Universe *old_universe, Universe *new_universe);
  std::string Description(Universe *universe) const;
 private:
  static const unsigned int MAX_E131_UNIVERSE = 63999;
};


class E131InputPort: public BasicInputPort {
 public:
  E131InputPort(E131Device *parent, int id, ola::acn::E131Node *node,
                class PluginAdaptor *plugin_adaptor)
      : BasicInputPort(parent, id, plugin_adaptor),
        m_node(node),
        m_priority(ola::dmx::SOURCE_PRIORITY_DEFAULT) {
    SetPriorityMode(PRIORITY_MODE_INHERIT);
  }

  bool PreSetUniverse(Universe *old_universe, Universe *new_universe) {
    return m_helper.PreSetUniverse(old_universe, new_universe);
  }
  void PostSetUniverse(Universe *old_universe, Universe *new_universe);
  std::string Description() const {
    return m_helper.Description(GetUniverse());
  }
  const ola::DmxBuffer &ReadDMX() const { return m_buffer; }
  bool SupportsPriorities() const { return true; }
  uint8_t InheritedPriority() const { return m_priority; }

 private:
  ola::DmxBuffer m_buffer;
  ola::acn::E131Node *m_node;
  E131PortHelper m_helper;
  uint8_t m_priority;
};


class E131OutputPort: public BasicOutputPort {
 public:
  E131OutputPort(E131Device *parent, int id, ola::acn::E131Node *node)
      : BasicOutputPort(parent, id),
        m_preview_on(false),
        m_node(node) {
    m_last_priority = GetPriority();
  }

  ~E131OutputPort();

  bool PreSetUniverse(Universe *old_universe, Universe *new_universe) {
    return m_helper.PreSetUniverse(old_universe, new_universe);
  }
  void PostSetUniverse(Universe *old_universe, Universe *new_universe);
  std::string Description() const {
    return m_helper.Description(GetUniverse());
  }

  bool WriteDMX(const ola::DmxBuffer &buffer, uint8_t priority);

  void SetPreviewMode(bool preview_mode) { m_preview_on = preview_mode; }
  bool PreviewMode() const { return m_preview_on; }
  bool SupportsPriorities() const { return true; }

 private:
  bool m_preview_on;
  uint8_t m_last_priority;
  ola::DmxBuffer m_buffer;
  ola::acn::E131Node *m_node;
  E131PortHelper m_helper;
};
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131PORT_H_
