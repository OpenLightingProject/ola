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

class ArtNetPort: public Port<ArtNetDevice> {
  public:
    ArtNetPort(ArtNetDevice *parent, unsigned int id):
      Port<ArtNetDevice>(parent, id) {}

    bool SetUniverse(Universe *universe);
    bool WriteDMX(const DmxBuffer &data);
    const DmxBuffer &ReadDMX() const;
    bool IsOutput() const;
    string Description() const;
  private:
    mutable DmxBuffer m_buffer;
};
}  // artnet
}  // plugin
}  // ola
#endif  // PLUGINS_ARTNET_ARTNETPORT_H_
