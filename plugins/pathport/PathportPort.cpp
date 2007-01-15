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
 * pathportport.cpp
 * The Pathport plugin for lla
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <string.h>

#include <llad/universe.h>
#include <llad/logger.h>

#include "PathportPort.h"
#include "PathportDevice.h"
#include "common.h"

#define min(a,b) a<b?a:b


PathportPort::PathportPort(Device *parent, int id) : Port(parent, id) {
	// if this is an in port, setup the buffer
	if(can_read()) {
		m_len = DMX_LENGTH;
		m_buf = (uint8_t*) malloc(m_len * sizeof(uint8_t));

		// we should handle this better
		if(m_buf == NULL) 
			Logger::instance()->log(Logger::CRIT, "PathportPlugin: malloc failed");
		else
			memset(m_buf, 0x00, m_len);
	} else {
		m_buf = NULL;
		m_len = 0;
	}
}


PathportPort::~PathportPort() {
	if(can_read())
		free(m_buf);
}


int PathportPort::can_read() const {
	return ( get_id() >=0 && get_id() < PORTS_PER_DEVICE/2);
}


int PathportPort::can_write() const {
	return ( get_id() >= PORTS_PER_DEVICE/2 && get_id() < PORTS_PER_DEVICE);
}


/*
 * Write operation
 * 
 * @param	data	pointer to the dmx data
 * @param	length	the length of the data
 *
 */
int PathportPort::write(uint8_t *data, int length) {
	PathportDevice *dev = (PathportDevice*) get_device();
	Universe *uni = get_universe();

	if(!can_write())
		return -1;

	if(uni == NULL)
		return 0;

	printf("sending %i\n", uni->get_uid());
	
	if(pathport_send_dmx(dev->get_node(), uni->get_uid(), length, data)) {
		Logger::instance()->log(Logger::WARN, "ShownetPlugin: pathport_send_dmx failed %s", pathport_strerror());
		return -1;
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
int PathportPort::read(uint8_t *data, int length) {
	int len;
	
	if(!can_read()) 
		return -1;
	
	len = min(m_len, length);
	memcpy(data, m_buf, len);
	return len;
}


/*
 * Update the data buffer for this port
 *
 */
int PathportPort::update_buffer(const uint8_t *data, int length) {
	int len = min(DMX_LENGTH, length);

	// we can't update if this isn't a input port
	if(!can_read())
		return -1;
	
	Logger::instance()->log(Logger::DEBUG, "Pathport: Updating dmx buffer for port %d", length);
	memcpy(m_buf, data, len);

	dmx_changed();
	return 0;
}


/*
 * We override the set universe method to register our interest in
 * pathport universes
 */
int PathportPort::set_universe(Universe *uni) {
    PathportDevice *dev = (PathportDevice*) get_device();
    pathport_node node = dev->get_node();

	Universe *old = get_universe();

    Port::set_universe(uni);

	if(can_read()) {
		// Unregister our interest in this universe
	    if(old != NULL) {
			pathport_unregister_uni(node, old->get_uid());
			dev->port_map(old,NULL);
		}

		if(uni != NULL) {
			dev->port_map(uni,this);
			pathport_register_uni(node, uni->get_uid());
		}
    }
    return 0;
}
