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
 * The Art-Net plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include <stdio.h>
#include <string.h>

#include "dummyport.h"

#define min(a,b) a<b?a:b

/*
 *
 *
 */
DummyPort::DummyPort(Device *parent, int id) : Port(parent, id) {
	memset(m_dmx, 0x00, 512) ;
	m_length = 512 ;
}

/*
 * Write operation
 * 
 * @param	data	pointer to the dmx data
 * @param	length	the length of the data
 *
 */
int DummyPort::write(uint8_t *data, int length) {
	int len = min(length, 512) ;
	
	memcpy(m_dmx, data, len) ;
	m_length = len ;

	printf("Dummy port: got %d bytes: 0x%hhx 0x%hhx 0x%hhx 0x%hhx \n", length, data[0], data[1], data[42], data[43] ) ;

	return 0;
}

/*
 * Read operation
 *
 * @param 	data	buffer to read data into
 * @param 	length	length of data to read
 *
 * @return	the amount of data read
 */
int DummyPort::read(uint8_t *data, int length) {
	int len ;

	// copy to mem
	len = min(length, m_length) ;
	memcpy(data, m_dmx, len) ;

	return len;
}
