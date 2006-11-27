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
 * eartnetdevice.cpp
 * Art-Net device
 * Copyright (C) 2005  Simon Newton
 *
 * An Art-Net device is an instance of libeartnet bound to a single IP address
 * Art-Net is limited to four ports per direction per IP, so in this case
 * our device has 8 ports :
 *
 * Ids 0-3 : Input ports (recv dmx)
 * Ids 4-7 : Output ports (send dmx)
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "eartnetdevice.h"
#include "eartnetport.h"

#include <llad/logger.h>
#include <llad/preferences.h>
#include <eartnet/eartnet.h>


#if HAVE_CONFIG_H
#  include <config.h>
#endif


/*
 * Handle dmx from the network, called from libeartnet
 * 
 */
int dmx_handler(eartnet_node n, int prt, void *d) {
	eArtNetDevice *dev = (eArtNetDevice *) d ;
	eArtNetPort *port ;
	
	// don't return non zero here else libeartnet will stop processing
	// this should never happen anyway
	if( prt < 0 || prt > EARTNET_MAX_PORTS) 
		return 0 ;

	// signal to the port that the data has changed
	port = (eArtNetPort*) dev->get_port(prt) ;
	port->dmx_changed() ;
	
	return 0;
}


/*
 * get notify of remote programming
 *
 *
 *
 */
int program_handler(eartnet_node n, void *d) {
	eArtNetDevice *dev = (eArtNetDevice *) d ;

	dev->save_config() ;
	return 0;
}


/*
 * Create a new device
 *
 * should prob pass the ip to bind to
 *
 */
eArtNetDevice::eArtNetDevice(Plugin *owner, const string &name, Preferences *prefs) : Device(owner, name) {
	m_prefs = prefs ;
	m_node = NULL ;
	m_enabled = false ;
}


/*
 *
 */
eArtNetDevice::~eArtNetDevice() {
	if (m_enabled)
		stop() ;
}


/*
 * Start this device
 *
 */
int eArtNetDevice::start() {
	eArtNetPort *port = NULL ;
	Port *prt = NULL;
	int debug = 0 ;
	
	/* set up ports */
	for(int i=0; i < 2*EARTNET_MAX_PORTS; i++) {
		port = new eArtNetPort(this,i) ;

		if(port != NULL) 
			this->add_port(port) ;
	}

#ifdef DEBUG
	debug = 1;
#endif
	
	// create new eartnet node, and and set config values

    if(m_prefs->get_val("ip") == "")
		m_node = eartnet_new(NULL, debug) ;
	else {
		m_node = eartnet_new(m_prefs->get_val("ip").c_str(), debug) ;
	}
	
	if(!m_node) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_new failed %s", eartnet_strerror() ) ;
		goto e_dev ;
	}

	// node config
	if(eartnet_setoem(m_node, 0x04, 0x31)  ) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_setoem failed: %s", eartnet_strerror()) ;
		goto e_eartnet_start ;
	}

	
	if(eartnet_set_short_name(m_node, m_prefs->get_val("short_name").c_str()) ) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_set_short_name failed: %s", eartnet_strerror()) ;
		goto e_eartnet_start ;
	}
	
	if (eartnet_set_long_name(m_node, m_prefs->get_val("long_name").c_str()) ) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_set_long_name failed: %s", eartnet_strerror()) ;
		goto e_eartnet_start ;
	}
	
	if(eartnet_set_node_type(m_node, EARTNET_SRV)) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_set_node_type failed: %s", eartnet_strerror()) ;
		goto e_eartnet_start ;
	}
	
	if(eartnet_set_subnet_addr(m_node, atoi(m_prefs->get_val("subnet").c_str()) ) ) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_set_subnet_addr failed: %s", eartnet_strerror()) ;
		goto e_eartnet_start ;
	}

	
	if(eartnet_set_dmx_handler(m_node, ::dmx_handler, (void*) this) ) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_set_dmx_handler failed: %s", eartnet_strerror()) ;
		goto e_eartnet_start ;
	}

	if(eartnet_start(m_node) ) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_start failed: %s", eartnet_strerror()) ;
		goto e_eartnet_start ;
	}
	m_enabled = true ;

	return 0;

e_eartnet_start:
	if(eartnet_destroy(m_node)) 
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_destroy failed: %s", eartnet_strerror()) ;
			
e_dev:
	for(int i=0; i < port_count() ; i++) {
		prt = get_port(i) ;
		if(prt != NULL) 
			delete prt ;
	}

	return -1 ;
}


/*
 * stop this device
 *
 */
int eArtNetDevice::stop() {
	Port *prt = NULL;

	if (!m_enabled)
		return 0 ;

	for(int i=0; i < port_count() ; i++) {
		prt = get_port(i) ;
		if(prt != NULL) 
			delete prt ;
	}

	if(eartnet_stop(m_node)) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_stop failed: %s", eartnet_strerror()) ;
		return -1 ;
	}
	
	if(eartnet_destroy(m_node)) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_destroy failed: %s", eartnet_strerror()) ;			
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
eartnet_node eArtNetDevice::get_node() const {
	return m_node ;
}

/*
 * return the sd of this device
 *
 */
int eArtNetDevice::get_sd() const {
	int ret ;
	
	ret = eartnet_get_sd(m_node) ;

	if(ret < 0) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_get_sd failed: %s", eartnet_strerror()) ;
		return -1 ;
	}
	return ret;
}

/*
 * Called when there is activity on our descriptors
 *
 * @param	data	user data (pointer to eartnet_device_priv
 */
int eArtNetDevice::fd_action() {
	if( eartnet_read(m_node, 0) ) {
		Logger::instance()->log(Logger::WARN, "eArtNetPlugin: eartnet_read failed: %s", eartnet_strerror()) ;
		return -1 ;
	}
	return 0;
}


// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/llad/llad.conf
int eArtNetDevice::save_config() {


	return 0;
}



/*
 * we can pass plugin specific messages here
 * make sure the user app knows the format though...
 *
 */
int eArtNetDevice::configure(void *req, int len) {
	// handle short/ long name & subnet and port addresses
	
	req = 0 ;
	len = 0;

	return 0;
}
