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
 * device.h
 * Header file for the Device class
 * Copyright (C) 2005  Simon Newton
 */

#ifndef DEVICE_H
#define DEVICE_H

class Plugin ;
class Port ;
#include <vector>
#include <string.h>

using namespace std;

class Device {

	public:

		Device(Plugin *Owner, const char *name) ;
		virtual ~Device() ;
		char *get_name() const ;
		Plugin	 	*get_owner() const ;
			
		// for the subclasses
		virtual int 	configure(void *req, int len) = 0;
		virtual int 	save_config() = 0;
		virtual int 	add_port(Port *prt) ;
		virtual Port	*get_port(int pid) const ;
		virtual int 	port_count() const ;


	private:
		Plugin *m_owner;					// which plugin owns this device
		int id;								// device id
		char *m_name;
		vector<Port*>	m_ports_vect ;		// ports on the device

};

#endif
