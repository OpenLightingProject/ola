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
 * The Art-Net plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include <stdlib.h>
#include <stdio.h>

#include <lla/pluginadaptor.h>

#include "opendmxplugin.h"
#include "opendmxdevice.h"

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
	
	/* create new lla device */
	m_dev = new OpenDmxDevice(this, "Open DMX USB Device") ;

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
	return 0;
}

char *OpenDmxPlugin::get_desc() {
		return 
"OpenDMXUSB Plugin\n"
"----------------------------\n"
"\n"
"The plugin creates a single device with one output port. "
"It expects to find the device at /dev/dmx0 . "
"This should be made configurable in the future..." ;
}
