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
 *
 * dummydevice.cpp
 * Art-Net device
 * Copyright (C) 2005  Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dummydevice.h"
#include "dummyport.h"

/*
 * Create a new dummy device
 *
 */
DummyDevice::DummyDevice(Plugin *owner, const char *name) : Device(owner, name) {
	m_enabled = false ;
}


/*
 * Stop this device
 *
 */
DummyDevice::~DummyDevice() {
	if (m_enabled)
		this->stop() ;

}


/*
 * Start this device
 *
 */
int DummyDevice::start() {
	DummyPort *port = NULL;

	if(m_enabled)
		return -1 ;

	port = new DummyPort(this, 0) ;

	if(port == NULL)
		return -1 ;

	if (add_port(port)) {
		delete port ;
		return -1 ;
	}

	m_enabled = true ;
	return 0 ;
}


/*
 * Stop this device
 *
 */
int DummyDevice::stop() {
	Port *prt = NULL;
	
	if (!m_enabled)
		return 0 ;

	for(int i=0; i < port_count() ; i++) {
		prt = get_port(i) ;
		if(prt != NULL) 
			delete prt ;
	}

	m_enabled = false ;
	return 0;
}


// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/llad/llad.conf
int DummyDevice::save_config() {


	return 0;
}


/*
 * we can pass plugin specific messages here
 * make sure the user app knows the format though...
 *
 */
int DummyDevice::configure(void *req, int len) {
	// handle short/ long name & subnet and port addresses
	
	req = 0 ;
	len = 0;

	return 0;
}

