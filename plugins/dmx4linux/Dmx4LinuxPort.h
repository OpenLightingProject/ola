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
 * dmx4linuxport.h
 * The Dmx4Linux plugin for lla
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef DMX4LINUXPORT_H
#define DMX4LINUXPORT_H

#include <llad/port.h>

class Dmx4LinuxPort : public Port {

  public:
    Dmx4LinuxPort(Device *parent, int id, int d4l, bool in, bool out);

    int write(uint8_t *data, unsigned int length);
    int read(uint8_t *data, unsigned int length);

    int can_read() const { return m_in; }
    int can_write() const { return m_out; }
  private:
    bool m_in;
    bool m_out;
    int m_d4l_uni;       // dmx4linux universe that this maps to
};

#endif
