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
 * DummyPort_h
 * Dummy port
 * Copyright (C) 2005-2007 Simon Newton
 */

#ifndef DUMMYPORT_H
#define DUMMYPORT_H

#include <llad/Port.h>
#include <lla/BaseTypes.h>

namespace lla {
namespace plugin {

class DummyPort: public Port {
  public:
    DummyPort(AbstractDevice *parent, int id);
    bool CanRead() { return false; }

    int WriteDMX(uint8_t *data, unsigned int length);
    int ReadDMX(uint8_t *data, unsigned int length);

  private:
    uint8_t m_dmx[DMX_UNIVERSE_SIZE]; // pointer to our dmx buffer
    unsigned int m_length;       // length of dmx buffer
};

} //plugin
} //lla
#endif
