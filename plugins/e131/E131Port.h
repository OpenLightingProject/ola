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
 * E131Port.h
 * The E1.31 port for OLA
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef PLUGINS_E131_E131PORT_H_
#define PLUGINS_E131_E131PORT_H_

#include <string>
#include "olad/Port.h"
#include "plugins/e131/E131Device.h"
#include "plugins/e131/e131/E131Node.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::DmxBuffer;

static const unsigned int NUMBER_OF_E131_PORTS = 5;

class E131PortHelper {
  public:
    bool PreSetUniverse(Universe *new_universe, Universe *old_universe);
    string Description(Universe *universe) const;
};


class E131InputPort: public InputPort {
  public:
    E131InputPort(E131Device *parent, int id, E131Node *node)
        : InputPort(parent, id),
          m_node(node) {}

    bool PreSetUniverse(Universe *new_universe, Universe *old_universe) {
      return m_helper.PreSetUniverse(new_universe, old_universe);
    }
    void PostSetUniverse(Universe *new_universe, Universe *old_universe);
    string Description() const { return m_helper.Description(GetUniverse()); }
    const DmxBuffer &ReadDMX() const { return m_buffer; }

  private:
    DmxBuffer m_buffer;
    E131Node *m_node;
    E131PortHelper m_helper;
};


class E131OutputPort: public OutputPort {
  public:
    E131OutputPort(E131Device *parent, int id, E131Node *node)
        : OutputPort(parent, id),
          m_node(node) {}

    bool PreSetUniverse(Universe *new_universe, Universe *old_universe) {
      return m_helper.PreSetUniverse(new_universe, old_universe);
    }
    void PostSetUniverse(Universe *new_universe, Universe *old_universe);
    string Description() const { return m_helper.Description(GetUniverse()); }

    bool WriteDMX(const DmxBuffer &buffer);
    void UniverseNameChanged(const string &new_name);

  private:
    DmxBuffer m_buffer;
    E131Node *m_node;
    E131PortHelper m_helper;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131PORT_H_
