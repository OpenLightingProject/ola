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
 * dummyport.cpp
 * The Dummy plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include <stdio.h>
#include <string.h>

#include <lla/logger.h>

#include "dummyport.h"

/*
 * 
 * @param parent	the parent device of this port
 * @param id		the port id
 *
 */
DummyPort::DummyPort(Device *parent, int id) : Port(parent, id), m_length(512) {
	memset(m_dmx, 0x00, m_length) ;
}


/*
 * Write operation
 * 
 * @param	data	pointer to the dmx data
 * @param	length	the length of the data
 *
 */
int DummyPort::write(uint8_t *data, int length) {
	int len = length < 512 ? length : 512 ;
	
	memcpy(m_dmx, data, len) ;
	m_length = len ;

	Logger::instance()->log(Logger::INFO, "Dummy port: got %d bytes: 0x%hhx 0x%hhx 0x%hhx 0x%hhx \n", length, data[0], data[1], data[42], data[43] ) ;

	return 0;
}


/*
 * Read operation, this isn't used for now else we'd create loops
 *
 * @param 	data	buffer to read data into
 * @param 	length	length of data to read
 *
 * @return	the amount of data read
 */
int DummyPort::read(uint8_t *data, int length) {
	int len ;

	// copy to mem
	len = length < m_length ? length : m_length ;
	memcpy(data, m_dmx, len) ;

	return len;
}
