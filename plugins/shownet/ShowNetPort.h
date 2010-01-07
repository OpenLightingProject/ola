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
 * ShowNetPort.h
 * The ShowNet plugin for ola
 * Copyright (C) 2005-2009 Simon Newton
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

using ola::DmxBuffer;

class ShowNetInputPort: public InputPort {
  public:
    ShowNetInputPort(ShowNetDevice *parent,
                     unsigned int id,
                     ShowNetNode *node):
      InputPort(parent, id),
      m_node(node) {}
    ~ShowNetInputPort() {}

    string Description() const;
    const DmxBuffer &ReadDMX() const { return m_buffer; }
    bool PreSetUniverse(Universe *new_universe, Universe *old_universe);
    void PostSetUniverse(Universe *new_universe, Universe *old_universe);

  private :
    DmxBuffer m_buffer;
    ShowNetNode *m_node;
};


class ShowNetOutputPort: public OutputPort {
  public:
    ShowNetOutputPort(ShowNetDevice *parent,
                      unsigned int id,
                      ShowNetNode *node):
      OutputPort(parent, id),
      m_node(node) {}
    ~ShowNetOutputPort() {}

    bool PreSetUniverse(Universe *new_universe, Universe *old_universe);
    string Description() const;
    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  private:
    ShowNetNode *m_node;
};
}  // shownet
}  // plugin
}  // ola
#endif  // PLUGINS_SHOWNET_SHOWNETPORT_H_
