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
 * eartnetport.cpp
 * The Art-Net plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include "eartnetport.h"
#include "eartnetdevice.h"
#include <lla/universe.h>
#include <lla/logger.h>
#include <string.h>

#define min(a,b) a<b?a:b


int eArtNetPort::can_read() {
	// ports 0 to 3 are input
	return ( get_id()>=0 && get_id() <EARTNET_MAX_PORTS);
}

int eArtNetPort::can_write() {
	// ports 4 to 7 are output
	return ( get_id()>=EARTNET_MAX_PORTS && get_id() <2*EARTNET_MAX_PORTS);
}

/*
 * Write operation
 * 
 * @param	data	pointer to the dmx data
 * @param	length	the length of the data
 *
 */
int eArtNetPort::write(uint8_t *data, int length) {
	eArtNetDevice *dev = (eArtNetDevice*) get_device() ;
	int ret ;

	if( !can_write())
		return -1 ;
	
	if(eartnet_send_dmx(dev->get_node() , this->get_id()%4 , length, data) ) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_send_dmx failed %s", eartnet_strerror() ) ;
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
int eArtNetPort::read(uint8_t *data, int length) {
	uint8_t *dmx = NULL;
	int len ;
	eArtNetDevice *dev = (eArtNetDevice*) get_device() ;
	
	if( !can_read()) 
		return -1 ;
	
	dmx = eartnet_read_dmx(dev->get_node(), get_id(), &len) ;
	
	if(dmx == NULL) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_read_dmx failed %s", eartnet_strerror() ) ;
		return -1 ;
	}
	
	memcpy(data, dmx, len ) ;
	return len;
}

/*
 * We override the set universe method to reprogram our
 */
int eArtNetPort::set_universe(Universe *uni) {
	eArtNetDevice *dev = (eArtNetDevice*) get_device() ;
	eartnet_node node = dev->get_node() ;
	int id = get_id() ;
	
	Port::set_universe(uni) ;

	// this is a bit of a hack but currently in libeartnet there is no 
	// way to disable a port once it's been enabled.
	if(uni == NULL)
		return 0 ;
	
	// carefull here, a port that we read from (input) is actually
	// an eArtNet output port
	if(id >= 0 && id <= 3) {
		// input port
		if(eartnet_set_port_type(node, id, EARTNET_ENABLE_OUTPUT, EARTNET_PORT_DMX)) {
			Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_set_port_type failed %s", eartnet_strerror() ) ;
			return -1 ;
		}
		
		if( eartnet_set_port_addr(node, id, EARTNET_OUTPUT_PORT, uni->get_uid())) {
			Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_set_port_addr failed %s", eartnet_strerror() ) ;
			return -1 ;
		}
		
	} else if (id >= 4 && id <= 7) {
		if(eartnet_set_port_type(node, id-4, EARTNET_ENABLE_INPUT, EARTNET_PORT_DMX) ) {
			Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_set_port_type failed %s", eartnet_strerror() ) ;
			return -1 ;
		}	
		printf("patching eartnet input port %i\n" , uni->get_uid() ) ;
		if(eartnet_set_port_addr(node, id-4, EARTNET_INPUT_PORT, uni->get_uid() ) ) {
			Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_set_port_addr failed %s", eartnet_strerror() ) ;
			return -1 ;
		}
	}
	return 0 ;
}
