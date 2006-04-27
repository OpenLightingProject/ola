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
 * usbproplugin.cpp
 * The UsbPro plugin for lla
 * Copyright (C) 2006  Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <lla/pluginadaptor.h>
#include <lla/preferences.h>

#include "usbproplugin.h"
#include "usbprodevice.h"

#define USBPRO_DEVICE "/dev/ttyUSB0"

#include <vector> 


/*
 * Entry point to this plugin
 */
extern "C" Plugin* create(PluginAdaptor *pa) {
  return new UsbProPlugin(pa);
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
 * Multiple devices now supported
 */
int UsbProPlugin::start() {
	int sd;
	vector<string> *dev_nm_v ;
	vector<string>::iterator it ;
	UsbProDevice *dev ;
	
	if(m_enabled)
		return -1 ;
	
	// setup prefs
	m_prefs = load_prefs() ;

	if(m_prefs == NULL) 
		return -1 ;

	// fetch device listing
	dev_nm_v = m_prefs->get_multiple_val("device") ;

	// for each device
	for ( it = dev_nm_v->begin() ; it != dev_nm_v->end(); ++it) {

		/* create new lla device */
		dev = new UsbProDevice(this, "Enttec Usb Pro Device", *it) ;

		if(dev == NULL) 
			continue ;

		if(dev->start()) {
			delete dev ;
			continue ;
		}

		// register our descriptors, this should really be fatal
		if ((sd = dev->get_sd()) >= 0)
			m_pa->register_fd( sd, PluginAdaptor::READ, dev) ;
	
		m_pa->register_device(dev) ;

		m_devices.insert(m_devices.end(), dev) ;

	}

	delete dev_nm_v ;

	if(m_devices.size() > 0) {
		m_enabled = true ;
	}
	return 0;
}


/*
 * Stop the plugin
 *
 * @return 0 on sucess, -1 on failure
 */
int UsbProPlugin::stop() {
	UsbProDevice *dev ;
	int i = 0 ;
	
	if (!m_enabled)
		return -1 ;
	
	for ( i = 0 ; i < m_devices.size() ; i++) {
		dev = m_devices[i] ;
			
		m_pa->unregister_fd( dev->get_sd(), PluginAdaptor::READ)  ;

		// stop the device
		if (dev->stop())
			continue ;
		
		m_pa->unregister_device(dev) ;

		delete dev ;
	}
	
	m_devices.clear() ;
	m_enabled = false ;
	delete m_prefs ;

	return 0;
}

/*
 * return the description for this plugin
 *
 */
char *UsbProPlugin::get_desc() {
		return
"Enttec Usb Pro Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates devices with one input and one output port.\n"
"\n"
"--- Config file : lla-usbpro.conf ---\n"
"\n"
"device = /dev/ttyUSB0\n"
"The device to use. Multiple devices are allowed\n";
}



/*
 * load the plugin prefs and default to sensible values
 *
 */
Preferences *UsbProPlugin::load_prefs() {
	Preferences *prefs = new Preferences("usbpro") ;

	if(prefs == NULL)
		return NULL ;

	prefs->load() ;

	if( prefs->get_val("device") == "") {
		prefs->set_val("device", USBPRO_DEVICE) ;
		prefs->save() ;
	}

	// check if this saved correctly
	// we don't want to use it if null
	if( prefs->get_val("device") == "" ) { 
		delete prefs;
		return NULL ;
	}

	return prefs ;
}
