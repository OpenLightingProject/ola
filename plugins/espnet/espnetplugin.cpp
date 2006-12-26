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
 * espnetplugin.cpp
 * The Esp Net plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <llad/pluginadaptor.h>
#include <llad/preferences.h>

#include "espnetplugin.h"
#include "espnetdevice.h"

#define ESPNET_NAME "lla-EspNet" 
/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(const PluginAdaptor *pa) {
  return new EspNetPlugin(pa, LLA_PLUGIN_ESPNET);
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
 */
int EspNetPlugin::start() {
	
	if(m_enabled)
		return -1;
	
	// setup prefs
	if (load_prefs() != 0)
		return -1;

	/* create new lla device */
	m_dev = new EspNetDevice(this, "ESPNet Device", m_prefs);

	if(m_dev == NULL) 
		goto e_prefs;

	if(m_dev->start()) {
		goto e_dev;
	}

	// register our descriptors
	//
	m_pa->register_fd( m_dev->get_sd(), PluginAdaptor::READ, m_dev) ;
	m_pa->register_device(m_dev);

	m_enabled = true;
	return 0;

	e_dev:
		delete m_dev;
	e_prefs:
		delete m_prefs;
		return -1;

}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
int EspNetPlugin::stop() {
			
	if (!m_enabled)
		return -1;
	
	m_pa->unregister_fd( m_dev->get_sd(), PluginAdaptor::READ) ;

	// stop the device
	if (m_dev->stop())
		return -1;
	

	m_pa->unregister_device(m_dev);
	m_enabled = false;
	delete m_dev;
	delete m_prefs;
	return 0;
}

/*
 * return the description for this plugin
 *
 */
string EspNetPlugin::get_desc() const {
	return 
"EspNet Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with five input and five output ports.\n"
"\n"
"Esp supports up to 255 universes. As ESP has no notion of ports, we provide "
"a fixed number of ports which can be patched to any universe. When sending "
"data from a port, the data is addressed to the universe the port is patched "
"to. For example if port 0 is patched to universe 10, the data will be sent to "
"ESP universe 10.\n"
"\n"
"--- Config file : lla-espnet.conf ---\n"
"\n"
"ip = a.b.c.d\n"
"The ip address to bind to. If not specified it will use the first non-loopback ip.\n"
"\n"
"name = lla-EspNet\n"
"The name of the node.\n";

}

/*
 * load the plugin prefs and default to sensible values
 *
 */
int EspNetPlugin::load_prefs() {
	if( m_prefs != NULL)
		delete m_prefs;

	m_prefs = new Preferences("espnet");

	if(m_prefs == NULL)
		return -1;

	m_prefs->load();

	// we don't worry about ip here
	// if it's non existant it will choose one
	if( m_prefs->get_val("name") == "") {
		m_prefs->set_val("name",ESPNET_NAME);
		m_prefs->save();
	}

	// check if this save correctly
	// we don't want to use it if null
	if( m_prefs->get_val("name") == "" ) {
		delete m_prefs;
		return -1;
	}

	return 0;
}
