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
 * device.cpp
 * The provides operations on a lla_device.
 * Copyright (C) 2005  Simon Newton
 */

#include <lla/device.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/*
 * Allocate memory for a device. More ports can be added later if required
 *
 * @param	ports	the number of ports to allocate
 * @return	0 on sucess, -1 on failure
 *
 * this should rather return the number of ports allocated
 */
Device::Device(Plugin *owner) {
	int i;
	
	m_owner = owner ;
	
}

/*
 * Return the owner of this device
 *
 */
inline Plugin *Device::get_owner() const {
	return m_owner ;
}


/*
 * get a port on the device
 *
 *
 *
 */
Port *Device::get_port(int pid) const {
	return m_ports_vect[pid] ;
}

/*
 * get the number of ports on the device
 *
 *
 */
inline int Device::get_ports() const {
	return m_ports_vect.size() ;
}


/*
 * add a port
 *
 */
int Device::add_port(Port *prt) {
	m_ports_vect.push_back(prt) ;
	return 0;
}
