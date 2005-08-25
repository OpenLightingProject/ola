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
 * devicemanagersingleton.cpp
 * The provides operations on a lla_device.
 * Copyright (C) 2005  Simon Newton
 */

#include "devicemanager.h"

#include <stdio.h>
#include <lla/logger.h>

/*
 * Create a new DeviceManager object
 *
 *
 */
DeviceManager::DeviceManager() {}


/*
 *
 *
 */
DeviceManager::~DeviceManager() {


}


/*
 * Register this device, called from plugins
 * 
 * @param 	dev		pointer to the Device to register
 * @return 0 on sucess, -1 on failure
 *
 */
int DeviceManager::register_device(Device *dev) {

	m_dev_vect.push_back(dev);
	
	// installed ok
	Logger::instance()->log(Logger::INFO, "Installed device") ;

	return 0;
}


/*
 * Unregister this device, called from plugins
 * 
 * @param 	dev		pointer to the Device to unregister
 * @return 0 on sucess, -1 on failure
 *
 */
int DeviceManager::unregister_device(Device *dev) {
	vector<Device*>::iterator it ;

	for (it = m_dev_vect.begin(); it != m_dev_vect.end(); ++it) {
		if(*it  == dev) {
			m_dev_vect.erase(it) ;
			return 0;
		}
	}

	return 1;
}


inline int DeviceManager::get_dev_count() {
	return m_dev_vect.size();
}


Device *DeviceManager::get_dev(int id) {

	if(id < 0 || id >= m_dev_vect.size())
		return NULL ;
	
	return m_dev_vect[id] ;
}

