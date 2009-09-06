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
 * E131Port.h
 * The E1.31 port for OLA
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef OLA_E131PORT_H
#define OLA_E131PORT_H

#include <olad/Port.h>
#include "E131Device.h"
#include "e131/E131Node.h"

namespace ola {
namespace e131 {

using ola::DmxBuffer;

class E131Port: public Port<E131Device> {
  public:
    E131Port(E131Device *parent, int id):
      Port<E131Device>(parent, id) {}

    bool CanRead() const;
    bool CanWrite() const;
    string Description() const;
    bool WriteDMX(const DmxBuffer &buffer);
    const DmxBuffer &ReadDMX() const;

    //int set_universe(Universe *uni);

    static const int NUMBER_OF_PORTS = 5;
  private:
    DmxBuffer m_buffer;
};

} //plugin
} //ola
#endif
