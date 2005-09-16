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
 * espnetdevice.cpp
 * Esp-Net device
 * Copyright (C) 2005  Simon Newton
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "espnetdevice.h"
#include "espnetport.h"

#include <lla/logger.h>
#include <lla/preferences.h>

#include <lla/universe.h>

/*
 * Handle dmx from the network, called from libespnet
 * 
 * @param n		the espnet_node
 * @param uni	the universe this data is for
 * @param len	the length of the received data
 * @param data	pointer the the dmx data
 * @param d		pointer to our EspNetDevice
 *
 */
int dmx_handler(espnet_node n, uint8_t uid, int len, uint8_t *data, void *d) {

//	Logger::instance()->log(Logger::WARN, "EspNetPlugin: got data") ;
	EspNetDevice *dev = (EspNetDevice *) d ;
	EspNetPort *prt ;
	Universe *uni ;
	for(int i =0 ; i < dev->port_count(); i++) {
		prt = (EspNetPort*) dev->get_port(i) ;
		uni = prt->get_universe() ;
		if( prt->can_read() && uni != NULL && uni->get_uid() == uid) {
			prt->update_buffer(data,len) ;
		}
	}
	
	return 0;
}


/*
 * get notify of remote programming
 *
 *
 *
 */
int program_handler(espnet_node n, void *d) {
	EspNetDevice *dev = (EspNetDevice *) d ;

	dev->save_config() ;
	return 0;
}


/*
 * Create a new device
 *
 * should prob pass the ip to bind to
 *
 */
EspNetDevice::EspNetDevice(Plugin *owner, const char *name, Preferences *prefs) : Device(owner, name) {
	m_prefs = prefs ;
	m_node = NULL ;
	m_enabled = false ;
}


/*
 *
 */
EspNetDevice::~EspNetDevice() {
	if (m_enabled)
		stop() ;
}


/*
 * Start this device
 *
 */
int EspNetDevice::start() {
	EspNetPort *port ;

	/* set up ports */
	for(int i=0; i < 2*PORTS_PER_DEVICE; i++) {
		port = new EspNetPort(this,i) ;

		if(port != NULL) 
			this->add_port(port) ;
	}

	// create new espnet node, and set config values
	if(m_prefs->get_val("ip") == "")
		m_node = espnet_new(NULL, 1) ;
	else {
		m_node = espnet_new(m_prefs->get_val("ip").c_str(), 1) ;
	}

	if(!m_node) {
		Logger::instance()->log(Logger::WARN, "EspNetPlugin: espnet_new failed: %s", espnet_strerror()) ;
		return -1 ;
	}

	// setup node
	if (espnet_set_name(m_node, m_prefs->get_val("name").c_str() ) ) {
		Logger::instance()->log(Logger::WARN, "EspNetPlugin: espnet_set_name failed: %s", espnet_strerror()) ;
		goto e_espnet_start; 
	}
	
	if(espnet_set_type(m_node, ESPNET_NODE_TYPE_IO)) {
		Logger::instance()->log(Logger::WARN, "EspNetPlugin: espnet_set_type failed: %s", espnet_strerror()) ;
		goto e_espnet_start; 
	}
	
	// we want to be notified when the node config changes
	if(espnet_set_dmx_handler(m_node, ::dmx_handler, (void*) this) ) {
		Logger::instance()->log(Logger::WARN, "EspNetPlugin: espnet_set_dmx_handler failed: %s", espnet_strerror()) ;
		goto e_espnet_start; 
	}

	if(espnet_start(m_node) ) {
		Logger::instance()->log(Logger::WARN, "EspNetPlugin: espnet_start failed: %s", espnet_strerror()) ;
		goto e_espnet_start ;
	}
	
	m_enabled = true ;
	return 0;

e_espnet_start:
	if(espnet_destroy(m_node)) 
		Logger::instance()->log(Logger::WARN, "EspNetPlugin: espnet_destory failed: %s", espnet_strerror()) ;			
e_dev:
	return -1 ;
}


/*
 * stop this device
 *
 */
int EspNetDevice::stop() {
	Port *prt ;

	if (!m_enabled)
		return 0 ;

	for(int i=0; i < port_count() ; i++) {
		prt = get_port(i) ;
		if(prt != NULL) 
			delete prt ;
	}

	if(espnet_stop(m_node)) {
		Logger::instance()->log(Logger::WARN, "EspNetPlugin: espnet_stop failed: %s", espnet_strerror()) ;	
		return -1 ;
	}
	
	if(espnet_destroy(m_node)) {
		Logger::instance()->log(Logger::WARN, "EspNetPlugin: espnet_destroy failed: %s", espnet_strerror()) ;			
		return -1 ;
	}
	
	m_enabled = false ;

	return 0;
}


/*
 * return the Art-Net node associated with this device
 *
 *
 */
espnet_node EspNetDevice::get_node() const {
	return m_node ;
}

/*
 * return the sd of this device
 *
 */
int EspNetDevice::get_sd(int sd) const {
	int ret ;
	sd = sd==0?0:1;
	ret = espnet_get_sd(m_node,sd) ;

	if(ret < 0) {
		Logger::instance()->log(Logger::WARN, "EspNetPlugin: espnet_get_sd failed: %s", espnet_strerror()) ;
		return -1 ;
	}
	return ret;
}

/*
 * Called when there is activity on our descriptors
 *
 * @param	data	user data (pointer to espnet_device_priv
 */
int EspNetDevice::fd_action() {
	if (espnet_read(m_node, 0) ) {
		Logger::instance()->log(Logger::WARN, "EspNetPlugin: espnet_read failed: %s", espnet_strerror()) ;
		return -1 ;
	}
	return 0;
}


// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/llad/llad.conf
int EspNetDevice::save_config() {


	return 0;
}



/*
 * we can pass plugin specific messages here
 * make sure the user app knows the format though...
 *
 */
int EspNetDevice::configure(void *req, int len) {
	// handle short/ long name & subnet and port addresses
	
	req = 0 ;
	len = 0;

	return 0;
}
