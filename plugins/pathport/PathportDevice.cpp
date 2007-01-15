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
 * pathportdevice.cpp
 * Pathport device
 * Copyright (C) 2005  Simon Newton
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <llad/logger.h>
#include <llad/preferences.h>
#include <llad/universe.h>

#include "PathportDevice.h"
#include "PathportPort.h"
#include "common.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif


/*
 * Handle dmx from the network, called from libpathport
 * 
 * @param n		the pathport_node
 * @param uni	the universe this data is for
 * @param len	the length of the received data
 * @param data	pointer the the dmx data
 * @param d		pointer to our PathportDevice
 *
 */
int dmx_handler(pathport_node n, unsigned int uid, unsigned int len, const uint8_t *data, void *d) {

	PathportDevice *dev = (PathportDevice *) d;
	PathportPort *prt;
	Universe *uni;

	if ( uid > PATHPORT_MAX_UNIVERSES) 
		return 0;
	
	prt = (PathportPort*) dev->get_port_from_uni(uid);
	uni = prt->get_universe();

	if( prt != NULL && prt->can_read() && uni != NULL) {
		prt->update_buffer(data, len);
	}

	n = NULL;
	return 0;
}



/*
 * Create a new device
 *
 * should prob pass the ip to bind to
 *
 */
PathportDevice::PathportDevice(Plugin *owner, const string &name, Preferences *prefs) :
	Device(owner, name),
	m_prefs(prefs),
	m_node(NULL),
	m_enabled(false) {

}


/*
 *
 */
PathportDevice::~PathportDevice() {
	if (m_enabled)
		stop();
}


/*
 * Start this device
 *
 */
int PathportDevice::start() {
	PathportPort *port = NULL;
	int debug = 0;
	
	/* set up ports */
	for(int i=0; i < PORTS_PER_DEVICE; i++) {
		port = new PathportPort(this, i);

		if(port != NULL) 
			this->add_port(port);
	}

#ifdef DEBUG
	debug = 1;
#endif
	
	// create new pathport node, and set config values
    if(m_prefs->get_val("ip") == "")
		m_node = pathport_new(NULL, debug);
	else {
		m_node = pathport_new(m_prefs->get_val("ip").c_str(), debug);
	}

	if(!m_node) {
		Logger::instance()->log(Logger::WARN, "PathportPlugin: pathport_new failed: %s", pathport_strerror());
		return -1;
	}

	// setup node
	if (pathport_set_name(m_node, m_prefs->get_val("name").c_str()) ) {
		Logger::instance()->log(Logger::WARN, "PathportPlugin: pathport_set_name failed: %s", pathport_strerror());
		goto e_pathport_start; 
	}

	// setup node
	if (pathport_set_type(m_node, PATHPORT_MANUF_ZP_TECH, PATHPORT_CLASS_NODE, PATHPORT_CLASS_NODE_PATHPORT) ) {
		Logger::instance()->log(Logger::WARN, "PathportPlugin: pathport_set_type failed: %s", pathport_strerror());
		goto e_pathport_start; 
	}

	// we want to be notified when the node config changes
	if(pathport_set_dmx_handler(m_node, ::dmx_handler, (void*) this) ) {
		Logger::instance()->log(Logger::WARN, "PathportPlugin: pathport_set_dmx_handler failed: %s", pathport_strerror());
		goto e_pathport_start; 
	}

	if(pathport_start(m_node) ) {
		Logger::instance()->log(Logger::WARN, "PathportPlugin: pathport_start failed: %s", pathport_strerror());
		goto e_pathport_start;
	}
	
	m_enabled = true;
	return 0;

e_pathport_start:
	if(pathport_destroy(m_node)) 
		Logger::instance()->log(Logger::WARN, "PathportPlugin: pathport_destory failed: %s", pathport_strerror());			
	return -1;
}


/*
 * stop this device
 *
 */
int PathportDevice::stop() {
	Port *prt = NULL;

	if (!m_enabled)
		return 0;

	for(int i=0; i < port_count(); i++) {
		prt = get_port(i);
		if(prt != NULL) 
			delete prt;
	}

	if(pathport_stop(m_node)) {
		Logger::instance()->log(Logger::WARN, "PathportPlugin: pathport_stop failed: %s", pathport_strerror());	
		return -1;
	}
	
	if(pathport_destroy(m_node)) {
		Logger::instance()->log(Logger::WARN, "PathportPlugin: pathport_destroy failed: %s", pathport_strerror());			
		return -1;
	}
	
	m_enabled = false;

	return 0;
}


/*
 * return the Art-Net node associated with this device
 *
 *
 */
pathport_node PathportDevice::get_node() const {
	return m_node;
}

/*
 * return the sd of this device
 *
 */
int PathportDevice::get_sd(unsigned int i) const {
	int ret = pathport_get_sd(m_node, i);

	if(ret < 0) {
		Logger::instance()->log(Logger::WARN, "PathportPlugin: pathport_get_sd failed: %s", pathport_strerror());
		return -1;
	}
	return ret;
}

/*
 * Called when there is activity on our descriptors
 *
 * @param	data	user data (pointer to pathport_device_priv
 */
int PathportDevice::fd_action() {
	if (pathport_read(m_node, 0) ) {
		Logger::instance()->log(Logger::WARN, "PathportPlugin: pathport_read failed: %s", pathport_strerror());
		return -1;
	}
	return 0;
}


// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/llad/llad.conf
int PathportDevice::save_config() const {


	return 0;
}



/*
 * we can pass plugin specific messages here
 * make sure the user app knows the format though...
 *
 */
int PathportDevice::configure(void *req, int len) {
	// handle short/ long name & subnet and port addresses
	
	req = 0;
	len = 0;

	return 0;
}


/*
 * Add a port to our hash map
 */
int PathportDevice::port_map(Universe *uni, PathportPort *prt) {
    m_portmap[uni->get_uid()] = prt;
    return 0;
}

PathportPort *PathportDevice::get_port_from_uni(int uni) {
    return m_portmap[uni];
}

