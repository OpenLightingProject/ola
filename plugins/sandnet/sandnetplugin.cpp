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
 * sandnetplugin.cpp
 * The SandNet plugin for lla
 * Copyright (C) 2005-2006  Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <llad/pluginadaptor.h>
#include <llad/preferences.h>

#include "sandnetplugin.h"
#include "sandnetdevice.h"

#define SANDNET_NAME "lla-SandNet"


/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(const PluginAdaptor *pa) {
  return new SandNetPlugin(pa, LLA_PLUGIN_SANDNET);
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
int SandNetPlugin::start() {
	
	if(m_enabled)
		return -1;
	
	// setup prefs
	if (load_prefs() != 0)
		return -1;

	/* create new lla device */
	m_dev = new SandNetDevice(this, "SandNet Device", m_prefs);

	if(m_dev == NULL) 
		goto e_prefs;

	if(m_dev->start())
		goto e_dev;
	
	// register our descriptors
	m_pa->register_fd( m_dev->get_sd(0), PluginAdaptor::READ, m_dev) ;
	m_pa->register_fd( m_dev->get_sd(1), PluginAdaptor::READ, m_dev) ;

	// timeout to send an advertisment every 2 seconds
	m_pa->register_timeout(2, m_dev);
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
int SandNetPlugin::stop() {
			
	if (!m_enabled)
		return -1;
	
	m_pa->unregister_fd( m_dev->get_sd(0), PluginAdaptor::READ) ;
	m_pa->unregister_fd( m_dev->get_sd(1), PluginAdaptor::READ) ;

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
string SandNetPlugin::get_desc() const {
	return 
"SandNet Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with 2 input and 8 output ports.\n"
"\n"
"The universe bindings are offset by one from those displayed in sandnet. For example, "
"sandnet universe 1 is lla universe 0\n"
"\n"
"--- Config file : lla-sandnet.conf ---\n"
"\n"
"ip = a.b.c.d\n"
"The ip to listen for sandnet traffic on. If not specified it will use the first non-loopback ip.\n"
"\n"
"name = lla-SandNet\n"
"The name of the node.\n";

}


/*
 * load the plugin prefs and default to sensible values
 *
 */
int SandNetPlugin::load_prefs() {
	if (m_prefs != NULL)
		delete m_prefs;

	m_prefs = new Preferences("sandnet");

	if(m_prefs == NULL)
		return -1;

	m_prefs->load();

	// we don't worry about ip here
	// if it's non existant it will choose one
	if( m_prefs->get_val("name") == "") {
		m_prefs->set_val("name", SANDNET_NAME);
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
