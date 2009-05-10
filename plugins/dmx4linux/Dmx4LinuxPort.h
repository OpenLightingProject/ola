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
 * Dmx4LinuxPort.h
 * The Dmx4Linux plugin for lla
 * Copyright (C) 2006-2008 Simon Newton
 */

#ifndef DMX4LINUXPORT_H
#define DMX4LINUXPORT_H

#include <lla/DmxBuffer.h>
#include <llad/Port.h>
#include "Dmx4LinuxDevice.h"

namespace lla {
namespace plugin {

class Dmx4LinuxPort: public lla::Port {
  public:
    Dmx4LinuxPort(Dmx4LinuxDevice *parent, int d4l, bool in, bool out);

    bool WriteDMX(const DmxBuffer &buffer);
    const DmxBuffer &ReadDMX() const;

    bool CanRead() const { return m_in; }
    bool CanWrite() const { return m_out; }
  private:
    bool m_in;
    bool m_out;
    int m_dmx_universe;  // dmx4linux universe that this maps to
    Dmx4LinuxDevice *m_device;
    DmxBuffer m_read_buffer;
};

} //plugin
} //lla

#endif
