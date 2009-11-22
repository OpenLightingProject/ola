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

class ShowNetPort: public Port<ShowNetDevice> {
  public:
    ShowNetPort(ShowNetDevice *parent, unsigned int id):
      Port<ShowNetDevice>(parent, id) {}
    ~ShowNetPort() {}

    bool IsOutput() const;
    string Description() const;
    bool WriteDMX(const DmxBuffer &buffer);
    const DmxBuffer &ReadDMX() const;
    bool SetUniverse(Universe *universe);
    int UpdateBuffer();

  private :
    DmxBuffer m_buffer;
    unsigned int ShowNetUniverseId() const { return PortId() / 2; }
};
}  // shownet
}  // plugin
}  // ola
#endif  // PLUGINS_SHOWNET_SHOWNETPORT_H_
