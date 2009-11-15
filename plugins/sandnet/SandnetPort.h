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

using ola::DmxBuffer;

class SandNetPort: public ola::Port<SandNetDevice> {
  public:
    SandNetPort(SandNetDevice *parent, unsigned int id):
      Port<SandNetDevice>(parent, id) {}
    ~SandNetPort() {}

    bool IsOutput() const;
    string Description() const;
    bool WriteDMX(const DmxBuffer &buffer);
    const DmxBuffer &ReadDMX() const { return m_buffer; }
    bool SetUniverse(Universe *universe);
    int UpdateBuffer();

  private:
    DmxBuffer m_buffer;
    uint8_t SandnetGroup(const Universe* universe) const;
    uint8_t SandnetUniverse(const Universe *universe) const;
};

}  // sandnet
}  // plugin
}  // ola

#endif  // PLUGINS_SANDNET_SANDNETPORT_H_
