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
 * Port.cpp
 * Base implementation of the port class
 * Copyright (C) 2005  Simon Newton
 */

#include <llad/Port.h>
#include <llad/Universe.h>

namespace lla {

/*
 * Create a new port
 *
 * @param parent  the device that owns this port
 * @param port_id    the port id of this port
 */
Port::Port(AbstractDevice *parent, int port_id):
  AbstractPort(),
  m_port_id(port_id),
  m_universe(NULL),
  m_parent(parent) {
}

/*
 * Signal that the data for this port has changed
 */
int Port::DmxChanged() {
  if(m_universe)
    return m_universe->PortDataChanged(this);
  return 0;
}

} //lla
