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
#include <fcntl.h>


#include <llad/pluginadaptor.h>
#include <llad/preferences.h>

#include "opendmxplugin.h"
#include "opendmxdevice.h"

#define DEFAULT_PATH "/dev/dmx0"

/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(const PluginAdaptor *pa) {
  return new OpenDmxPlugin(pa, LLA_PLUGIN_OPEN);
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
 * 	Need to get multi-device support working first :)
 */
int OpenDmxPlugin::start() {
	int fd;
	
	if (m_enabled)
		return -1;
	
	// setup prefs
	if (load_prefs() != 0)
		return -1;
	
	/* create new lla device */
	// first check if it's there
	fd = open( m_prefs->get_val("device").c_str(), O_WRONLY);
	
	if ( fd > 0 ) {
		close(fd);
		m_dev = new OpenDmxDevice(this, "Open DMX USB Device", m_prefs->get_val("device"));

		if (m_dev == NULL) {
			delete m_prefs;
			return 0;
		}

		m_dev->start();

		m_pa->register_device(m_dev);

		m_enabled = true;
		return 0;
	} else {
		delete m_prefs;
		return 0;
	}
}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
int OpenDmxPlugin::stop() {
			
	if (!m_enabled)
		return -1;

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
string OpenDmxPlugin::get_desc() const {
		return 
"OpenDMXUSB Plugin\n"
"----------------------------\n"
"\n"
"The plugin creates a single device with one output port using "
"the Enttec Open DMX USB widget.\n"
"\n"
"--- Config file : lla-opendmx.conf ---\n"
"\n"
"device = " DEFAULT_PATH "\n"
"The path to the open dmx usb device.\n";
}


/*
 * load the plugin prefs and default to sensible values
 *
 */
int OpenDmxPlugin::load_prefs() {

	if (m_prefs != NULL)
		delete m_prefs;

	m_prefs = new Preferences("opendmx");

	if (m_prefs == NULL)
		return -1;

	m_prefs->load();

	if (m_prefs->get_val("device") == "") {
		m_prefs->set_val("device", DEFAULT_PATH);
		m_prefs->save();
	}

	// check if this save correctly
	// we don't want to use it if null
	if (m_prefs->get_val("device") == "") {
		delete m_prefs;
		return NULL;
	}

	return 0;
}
