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
 * opendmxplugin.cpp
 * The Open DMX plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <lla/pluginadaptor.h>
#include <lla/preferences.h>

#include "opendmxplugin.h"
#include "opendmxdevice.h"

#define DEFAULT_PATH "/dev/dmx0"

/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(PluginAdaptor *pa) {
  return new OpenDmxPlugin(pa);
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
 * TODO: scan /dev for devices?
 */
int OpenDmxPlugin::start() {
	
	if(m_enabled)
		return -1 ;
	
	// setup prefs
	m_prefs = load_prefs() ;

	if(m_prefs == NULL) 
		return -1 ;
	
	/* create new lla device */
	// should we maybe be making a copy of the string here ?
	m_dev = new OpenDmxDevice(this, "Open DMX USB Device", m_prefs->get_val("device")) ;

	if(m_dev == NULL) 
		return -1  ;

	m_dev->start() ;

	m_pa->register_device(m_dev) ;

	m_enabled = true ;
	return 0;
}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
int OpenDmxPlugin::stop() {
			
	if (!m_enabled)
		return -1 ;

	// stop the device
	if (m_dev->stop())
		return -1 ;
		
	m_pa->unregister_device(m_dev) ;
	m_enabled = false ;
	delete m_dev ;
	delete m_prefs ;

	return 0;
}

/*
 * return the description for this plugin
 *
 */
char *OpenDmxPlugin::get_desc() {
		return 
"OpenDMXUSB Plugin\n"
"----------------------------\n"
"\n"
"The plugin creates a single device with one output port using\n"
"the Enttec Open DMX USB widget.\n"
"\n"
"--- Options ---\n"
"\n"
"The path to the device is controlled with the following line:\n"
"	device = " DEFAULT_PATH "\n"
"in the lla-opendmx.conf file." ;
}


/*
 * load the plugin prefs and default to sensible values
 *
 */
Preferences *OpenDmxPlugin::load_prefs() {
	Preferences *prefs = new Preferences("opendmx") ;

	if(prefs == NULL)
		return NULL ;

	prefs->load() ;

	if( prefs->get_val("device") == "") {
		prefs->set_val("device",DEFAULT_PATH) ;
		prefs->save() ;
	}

	return prefs ;
}
