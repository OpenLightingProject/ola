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
 * The Dmx4Linux plugin for ola
 * Copyright (C) 2006-2008 Simon Newton
 */

#ifndef PLUGINS_DMX4LINUX_DMX4LINUXPORT_H_
#define PLUGINS_DMX4LINUX_DMX4LINUXPORT_H_

#include <string>
#include "ola/BaseTypes.h"
#include "ola/DmxBuffer.h"
#include "plugins/dmx4linux/Dmx4LinuxDevice.h"
#include "plugins/dmx4linux/Dmx4LinuxSocket.h"

namespace ola {
namespace plugin {
namespace dmx4linux {


/*
 * A Dmx4Linux output port, we only have 1 port per device so the port id is
 * always 0.
 */
class Dmx4LinuxOutputPort: public BasicOutputPort {
  public:
    Dmx4LinuxOutputPort(Dmx4LinuxDevice *parent,
                        Dmx4LinuxSocket *socket,
                        int d4l_universe) :
        BasicOutputPort(parent, 0),
        m_socket(socket),
        m_d4l_universe(d4l_universe) {
    }

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    string Description() const { return ""; }

  private:
    Dmx4LinuxSocket *m_socket;
    int m_d4l_universe;  // dmx4linux universe that this maps to
};


/*
 * A Dmx4Linux input port, we only have 1 port per device so the port id is
 * always 0.
 */
class Dmx4LinuxInputPort: public BasicInputPort {
  public:
    explicit Dmx4LinuxInputPort(Dmx4LinuxDevice *parent,
                                const TimeStamp *wake_time):
        BasicInputPort(parent, 0, wake_time) {
      m_read_buffer.SetRangeToValue(0, 0, DMX_UNIVERSE_SIZE);
    }

    const DmxBuffer &ReadDMX() const;
    bool UpdateData(const uint8_t *in_buffer, unsigned int length);
    string Description() const { return ""; }

  private:
    DmxBuffer m_read_buffer;
};
}  // dmx4linux
}  // plugin
}  // ola
#endif  // PLUGINS_DMX4LINUX_DMX4LINUXPORT_H_
