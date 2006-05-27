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
 * usbproplugin.h
 * Interface for the usbpro plugin class
 * Copyright (C) 2006  Simon Newton
 */

#ifndef USBPROPLUGIN_H
#define USBPROPLUGIN_H

#include <vector>
#include <lla/plugin.h>
#include <lla/fdmanager.h>

class UsbProDevice ;

class UsbProPlugin : public Plugin, public FDManager {

	public:
		UsbProPlugin(PluginAdaptor *pa) : Plugin(pa) {m_enabled = false; }

		int start();
		int stop();
		bool is_enabled() 	{ return m_enabled; }
		char *get_name() 	{ return "UsbPro Plugin"; }
		char *get_desc() ;
		int fd_error(int error, FDListener *listener) ;
	private:
		Preferences *load_prefs() ;
		
		class Preferences *m_prefs ;
		vector<UsbProDevice *>  m_devices;		// list of out devices
		bool m_enabled ;		// are we running
};

#endif

