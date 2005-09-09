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
 * espnetport.cpp
 * The Art-Net plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include "espnetport.h"
#include "espnetdevice.h"
#include "common.h"

#include <lla/universe.h>
#include <lla/logger.h>

#include <string.h>

#define min(a,b) a<b?a:b

EspNetPort::EspNetPort(Device *parent, int id) : Port(parent, id) {

	// if this is an in port, setup the buffer
	if(can_read()) {
		m_len = DMX_LENGTH ;
		m_buf = (uint8_t*) malloc(m_len) ;

		// we should handle this better
		if(m_buf == NULL) 
			Logger::instance()->log(Logger::CRIT, "EspNetPlugin: malloc failed") ;
		else
			memset(m_buf, 0x00, m_len) ;
	} else {
		m_buf  = NULL ;
		m_len = 0;
	}

}

EspNetPort::~EspNetPort() {

	if(can_read())
		free(m_buf) ;
}

int EspNetPort::can_read() {
	// ports 0 to 4 are input
	return ( get_id()>=0 && get_id() < PORTS_PER_DEVICE);
}

int EspNetPort::can_write() {
	// ports 5 to 9 are output
	return ( get_id()>= PORTS_PER_DEVICE && get_id() <2*PORTS_PER_DEVICE);
}

/*
 * Write operation
 * 
 * @param	data	pointer to the dmx data
 * @param	length	the length of the data
 *
 */
int EspNetPort::write(uint8_t *data, int length) {
	EspNetDevice *dev = (EspNetDevice*) get_device() ;
	int ret ;

	if( !can_write())
		return -1 ;
	
	if(espnet_send_dmx(dev->get_node() , this->get_universe()->get_uid() , length, data)) {
		Logger::instance()->log(Logger::WARN, "EspNetPlugin: espnet_send_dmx failed %s", espnet_strerror() ) ;
		return -1 ;
	}
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
int EspNetPort::read(uint8_t *data, int length) {
	uint8_t *dmx ;
	int len ;
	EspNetDevice *dev = (EspNetDevice*) get_device() ;
	
	if( !can_read()) 
		return -1 ;
	
	len = min(m_len, length) ;
	memcpy(data, m_buf, len ) ;
	return len;
}

/*
 * Update the data buffer for this port
 *
 */
int EspNetPort::update_buffer(uint8_t *data, int length) {
	int len = min(DMX_LENGTH, length) ;

	// we can't update if this isn't a input port
	if(! can_read())
		return -1 ;
	
	Logger::instance()->log(Logger::DEBUG, "ESP: Updating dmx buffer for port %d", length);
	memcpy(m_buf, data, len);

	dmx_changed() ;
}
