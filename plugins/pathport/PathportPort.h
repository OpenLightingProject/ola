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
 * PathportPort.h
 * The Pathport plugin for ola
 * Copyright (C) 2005-2009 Simon Newton
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
    string Description(const Universe *universe) const;
    bool PreSetUniverse(Universe *new_universe);
};


class PathportInputPort: public InputPort {
  public:
    PathportInputPort(PathportDevice *parent,
                     unsigned int id,
                     PathportNode *node):
      InputPort(parent, id),
      m_node(node) {}
    ~PathportInputPort() {}

    string Description() const { return m_helper.Description(GetUniverse()); }
    const DmxBuffer &ReadDMX() const { return m_buffer; }
    bool PreSetUniverse(Universe *new_universe, Universe *old_universe) {
      return m_helper.PreSetUniverse(new_universe);
    }

  private:
    PathportPortHelper m_helper;
    PathportNode *m_node;
    DmxBuffer m_buffer;
};


class PathportOutputPort: public OutputPort {
  public:
    PathportOutputPort(PathportDevice *parent,
                      unsigned int id,
                      PathportNode *node):
      OutputPort(parent, id),
      m_node(node) {}
    ~PathportOutputPort() {}

    string Description() const { return m_helper.Description(GetUniverse()); }
    bool WriteDMX(const DmxBuffer &buffer);
    bool PreSetUniverse(Universe *new_universe, Universe *old_universe) {
      return m_helper.PreSetUniverse(new_universe);
    }

  private:
    PathportPortHelper m_helper;
    PathportNode *m_node;
};
}  // pathport
}  // plugin
}  // ola
#endif  // PLUGINS_PATHPORT_PATHPORTPORT_H_
