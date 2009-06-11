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
 * EspNetPort.h
 * The ESPNet plugin for lla
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef ESPNETPORT_H
#define ESPNETPORT_H

#include <lla/DmxBuffer.h>
#include <llad/Port.h>
#include "EspNetDevice.h"

namespace lla {
namespace espnet {

using lla::DmxBuffer;

class EspNetPort: public lla::Port {
  public:
    EspNetPort(EspNetDevice *parent, unsigned int id): Port(parent, id) {}
    ~EspNetPort() {}

    bool CanRead() const;
    bool CanWrite() const;
    string Description() const;
    bool WriteDMX(const DmxBuffer &buffer);
    const DmxBuffer &ReadDMX() const;
    bool SetUniverse(Universe *universe);
    int UpdateBuffer();

  private :
    DmxBuffer m_buffer;
    uint8_t EspNetUniverseId() const;
};

} //espnet
} //lla

#endif
