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
 * port.cpp
 * methods for the port class
 * Copyright (C) 2005  Simon Newton
 */

#include <lla/port.h>
#include <lla/universe.h>

#include <stdio.h>

Port::Port(Device *parent, int id) {
	m_pid = id ;
	m_fh = NULL ;
	m_fh_data = NULL ;
	m_universe = NULL ;
	m_parent = parent ;
}

int Port::dmx_changed() {
	if(m_universe)
		return m_universe->port_data_changed(this) ;
	return 0;
}

// register a dmx handler for this port
int Port::register_dmx(int (*fh)(void *data), void *data) {
	m_fh = fh;
	m_fh_data = data;
	return 0;
}
