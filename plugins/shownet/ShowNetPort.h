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
 * The ShowNet plugin for lla
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef SHOWNETPORT_H
#define SHOWNETPORT_H

#include <llad/Port.h>
#include <llad/Device.h>
#include "ShowNetNode.h"

namespace lla {
namespace shownet {

using lla::DmxBuffer;

class ShowNetPort: public Port {
  public:
    ShowNetPort(lla::Device *parent, unsigned int id): Port(parent, id) {}
    ~ShowNetPort() {}

    bool CanRead() const;
    bool CanWrite() const;
    string Description() const;
    bool WriteDMX(const DmxBuffer &buffer);
    const DmxBuffer &ReadDMX() const;
    bool SetUniverse(Universe *universe);
    int UpdateBuffer();

  private :
    DmxBuffer m_buffer;
    unsigned int ShowNetUniverseId() const { return PortId() / 2; }
};

} //plugin
} //lla
#endif
