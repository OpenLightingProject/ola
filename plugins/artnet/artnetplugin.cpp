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
 * artnetplugin.cpp
 * The Art-Net plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <lla/pluginadaptor.h>

#include "artnetplugin.h"
#include "artnetdevice.h"

/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(PluginAdaptor *pa) {
  return new ArtNetPlugin(pa);
}

/*
 * Called when the plugin is unloaded
 */
extern "C" void destroy(Plugin* plug) {
  delete plug;
}


/*
 * Start the plugin
 *
 * For now we just have one device.
 * TODO: allow multiple devices on different IPs ?
 */
int ArtNetPlugin::start() {
	
	if(m_enabled)
		return -1 ;
	
	/* create new lla device */
	m_dev = new ArtNetDevice(this, "Art-Net Device") ;

	if(m_dev == NULL) 
		return -1  ;

	m_dev->start() ;

	// register our descriptors
	m_pa->register_fd( m_dev->get_sd(0), PluginAdaptor::READ, m_dev)  ;
	m_pa->register_fd( m_dev->get_sd(1), PluginAdaptor::READ, m_dev)  ;

	m_pa->register_device(m_dev) ;

	m_enabled = true ;
	return 0;
}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
int ArtNetPlugin::stop() {
			
	if (!m_enabled)
		return -1 ;
	
	m_pa->unregister_fd( m_dev->get_sd(0), PluginAdaptor::READ)  ;
	m_pa->unregister_fd( m_dev->get_sd(1), PluginAdaptor::READ)  ;

	// stop the device
	if (m_dev->stop())
		return -1 ;
	

	m_pa->unregister_device(m_dev) ;
	m_enabled = false ;
	delete m_dev ;
	return 0;
}


char *ArtNetPlugin::get_desc() {
		return
"ArtNet Plugin\n"
"----------------------------\n"
"\n"
"The plugin creates a single device with four input and four output ports. "
"Currently this plugin binds to the first non-loopback IP. This should "
"be made configurable in the future...\n" ;
}
