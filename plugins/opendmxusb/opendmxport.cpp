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
 * opendmxport.cpp
 * The Open DMX plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include "opendmxport.h"
#include "opendmxdevice.h"

#include "string.h"

#define min(a,b) a<b?a:b

OpenDmxPort::OpenDmxPort(Device *parent, int id, string path) : Port(parent, id) {
	m_thread = new OpenDmxThread() ;
	m_thread->start(path) ;

}

#include <stdio.h>

OpenDmxPort::~OpenDmxPort() {
	m_thread->stop() ;
	delete m_thread;
}

/*
 * The read operation is not supported in drivers)
 *
 */
inline int OpenDmxPort::can_read() { return 0; }
inline int OpenDmxPort::read(uint8_t *data, int length) { return 0; } 


/*
 * Write operation
 * 
 * @param	data	pointer to the dmx data
 * @param	length	the length of the data
 *
 */
#include <stdio.h>

int OpenDmxPort::write(uint8_t *data, int length) {

	if( !can_write())
		return -1 ;

	m_thread->write_dmx(data,length) ;
	return 0;
}


