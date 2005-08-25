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
 * artnetport.cpp
 * The Art-Net plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include "artnetport.h"
#include "artnetdevice.h"
#include <lla/universe.h>

#include <string.h>

#define min(a,b) a<b?a:b


int ArtNetPort::can_read() {
	// ports 0 to 3 are input
	return ( get_id()>=0 && get_id() <ARTNET_MAX_PORTS);
}

int ArtNetPort::can_write() {
	// ports 4 to 7 are output
	return ( get_id()>=ARTNET_MAX_PORTS && get_id() <2*ARTNET_MAX_PORTS);
}

/*
 * Write operation
 * 
 * @param	data	pointer to the dmx data
 * @param	length	the length of the data
 *
 */
int ArtNetPort::write(uint8_t *data, int length) {
	ArtNetDevice *dev = (ArtNetDevice*) get_device() ;
	int ret ;

	if( !can_write())
		return -1 ;
	
	ret = artnet_send_dmx(dev->get_node() , this->get_id()/4 , length, data) ;
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
int ArtNetPort::read(uint8_t *data, int length) {
	uint8_t *dmx ;
	int len ;
	ArtNetDevice *dev = (ArtNetDevice*) get_device() ;
	
	if( !can_read()) 
		return -1 ;
	
	dmx = artnet_read_dmx(dev->get_node(), get_id(), &len) ;
	
	memcpy(data, dmx, len ) ;
	return len;
}

/*
 * We override the set universe method to reprogram our
 */
int ArtNetPort::set_universe(Universe *uni) {
	ArtNetDevice *dev = (ArtNetDevice*) get_device() ;
	artnet_node node = dev->get_node() ;
	int id = get_id() ;
	
	Port::set_universe(uni) ;

	// carefull here, a port that we read from (input) is actually
	// an ArtNet output port
	if(id >= 0 && id <= 3) {
		// input port
		artnet_set_port_type(node, id, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX) ;
		artnet_set_port_addr(node, id, ARTNET_OUTPUT_PORT, uni->get_uid()) ;
		
	} else if (id >= 4 && id <= 7) {
		artnet_set_port_type(node, id, ARTNET_ENABLE_INPUT, ARTNET_PORT_DMX) ;
		artnet_set_port_addr(node, id, ARTNET_INPUT_PORT, uni->get_uid() ) ;
	}

}
