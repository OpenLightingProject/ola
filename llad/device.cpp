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
 * Base implementation of the device class.
 * Copyright (C) 2005 Simon Newton
 */

#include <llad/device.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Create a new device
 *
 * @param  owner  the plugin that owns this device
 * @param  name  a nice name for this device
 */
Device::Device(Plugin *owner, const string &name)
  : m_owner(owner), m_name(name)  {

}

/*
 * Destroy this device
 */
Device::~Device() {}


/*
 * Return the plugin owner of this device
 *
 * @return  the plugin that owns this device
 */
Plugin *Device::get_owner() const {
  return m_owner;
}

/*
 * get the name of this device
 *
 * @return the name of the device
 */
const string Device::get_name() const {
  return m_name;
}

/*
 * Get a port on the device
 *
 * @param pid  the id of the port to fetch
 * @return the port if it exists, or NULL on error
 */
Port *Device::get_port(unsigned int pid) const {
  if (pid > m_ports_vect.size() )
    return NULL;

  return m_ports_vect[pid];
}

/*
 * Get the number of ports on the device
 *
 * @return the number of ports in this device
 */
inline int Device::port_count() const {
  return m_ports_vect.size();
}


/*
 * Add a port to this device
 *
 * @param prt  the port to add
 * @return 0 on success, non 0 on failure
 */
int Device::add_port(Port *prt) {
  m_ports_vect.push_back(prt);
  return 0;
}
