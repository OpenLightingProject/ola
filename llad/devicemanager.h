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
 * llad.cpp
 * The provides operations on a lla_device.
 * Copyright (C) 2005  Simon Newton
 */

#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <vector>

#include <lla/device.h> 

using namespace std;

class DeviceManager {

	public:
		DeviceManager();
		~DeviceManager();
		
		int register_device(Device *dev) ;
		int unregister_device(Device *dev) ;
		int get_dev_count() ;
		Device *get_dev(int id) ;

	protected:

	private:
		vector<Device*> m_dev_vect ;
} ;

#endif
