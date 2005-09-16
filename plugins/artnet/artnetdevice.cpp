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
 * artnetdevice.cpp
 * Art-Net device
 * Copyright (C) 2005  Simon Newton
 *
 * An Art-Net device is an instance of libartnet bound to a single IP address
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

#include "artnetdevice.h"
#include "artnetport.h"

#include <lla/logger.h>
#include <lla/preferences.h>
#include <artnet/artnet.h>

/*
 * Handle dmx from the network, called from libartnet
 * 
 */
int dmx_handler(artnet_node n, int prt, void *d) {
	ArtNetDevice *dev = (ArtNetDevice *) d ;
	ArtNetPort *port ;
	
	// don't return non zero here else libartnet will stop processing
	// this should never happen anyway
	if( prt < 0 || prt > ARTNET_MAX_PORTS) 
		return 0 ;

	// signal to the port that the data has changed
	port = (ArtNetPort*) dev->get_port(prt) ;
	port->dmx_changed() ;
	
	return 0;
}


/*
 * get notify of remote programming
 *
 *
 *
 */
int program_handler(artnet_node n, void *d) {
	ArtNetDevice *dev = (ArtNetDevice *) d ;

	dev->save_config() ;
	return 0;
}


/*
 * Create a new device
 *
 * should prob pass the ip to bind to
 *
 */
ArtNetDevice::ArtNetDevice(Plugin *owner, const char *name, Preferences *prefs) : Device(owner, name) {
	m_prefs = prefs ;
	m_node = NULL ;
	m_enabled = false ;
}


/*
 *
 */
ArtNetDevice::~ArtNetDevice() {
	if (m_enabled)
		stop() ;
}


/*
 * Start this device
 *
 */
int ArtNetDevice::start() {
	ArtNetPort *port ;

	/* set up ports */
	for(int i=0; i < 2*ARTNET_MAX_PORTS; i++) {
		port = new ArtNetPort(this,i) ;

		if(port != NULL) 
			this->add_port(port) ;
	}

	
	// create new artnet node, and and set config values

    if(m_prefs->get_val("ip") == "")
		m_node = artnet_new(NULL, 1) ;
	else {
		m_node = artnet_new(m_prefs->get_val("ip").c_str(), 1) ;
	}
	
	if(!m_node) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_new failed %s", artnet_strerror() ) ;
		return -1 ;
	}

	// node config
	if(artnet_set_short_name(m_node, m_prefs->get_val("short_name").c_str()) ) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_set_short_name failed: %s", artnet_strerror()) ;
		goto e_artnet_start ;
	}
	
	if (artnet_set_long_name(m_node, m_prefs->get_val("long_name").c_str()) ) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_set_long_name failed: %s", artnet_strerror()) ;
		goto e_artnet_start ;
	}
	
	if(artnet_set_node_type(m_node, ARTNET_SRV)) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_set_node_type failed: %s", artnet_strerror()) ;
		goto e_artnet_start ;
	}
	
	if(artnet_set_subnet_addr(m_node, atoi(m_prefs->get_val("subnet").c_str()) ) ) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_set_subnet_addr failed: %s", artnet_strerror()) ;
		goto e_artnet_start ;
	}

	// we want to be notified when the node config changes
	if(artnet_set_program_handler(m_node, ::program_handler, (void*) this) ) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_set_program_handler failed: %s", artnet_strerror()) ;
		goto e_artnet_start ;
	}
	
	if(artnet_set_dmx_handler(m_node, ::dmx_handler, (void*) this) ) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_set_dmx_handler failed: %s", artnet_strerror()) ;
		goto e_artnet_start ;
	}

	if(artnet_start(m_node) ) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_start failed: %s", artnet_strerror()) ;
		goto e_artnet_start ;
	}
	m_enabled = true ;

	return 0;

e_artnet_start:
	if(artnet_destroy(m_node)) 
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_destroy failed: %s", artnet_strerror()) ;
			
e_dev:
	return -1 ;
}


/*
 * stop this device
 *
 */
int ArtNetDevice::stop() {
	Port *prt ;

	if (!m_enabled)
		return 0 ;

	for(int i=0; i < port_count() ; i++) {
		prt = get_port(i) ;
		if(prt != NULL) 
			delete prt ;
	}

	if(artnet_stop(m_node)) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_stop failed: %s", artnet_strerror()) ;
		return -1 ;
	}
	
	if(artnet_destroy(m_node)) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_destroy failed: %s", artnet_strerror()) ;			
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
artnet_node ArtNetDevice::get_node() const {
	return m_node ;
}

/*
 * return the sd of this device
 *
 */
int ArtNetDevice::get_sd(int id) const {
	int ret ;
	id = id?1:0 ;
	
	ret = artnet_get_sd(m_node,id) ;

	if(ret < 0) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_get_sd failed: %s", artnet_strerror()) ;
		return -1 ;
	}
	return ret;
}

/*
 * Called when there is activity on our descriptors
 *
 * @param	data	user data (pointer to artnet_device_priv
 */
int ArtNetDevice::fd_action() {
	if( artnet_read(m_node, 0) ) {
		Logger::instance()->log(Logger::WARN, "ArtNetPlugin: artnet_read failed: %s", artnet_strerror()) ;
		return -1 ;
	}
	return 0;
}


// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/llad/llad.conf
int ArtNetDevice::save_config() {


	return 0;
}



/*
 * we can pass plugin specific messages here
 * make sure the user app knows the format though...
 *
 */
int ArtNetDevice::configure(void *req, int len) {
	// handle short/ long name & subnet and port addresses
	
	req = 0 ;
	len = 0;

	return 0;
}
