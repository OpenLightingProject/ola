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
 * sandnetplugin.h
 * Interface for the sandnet plugin class
 * Copyright (C) 2005-2006  Simon Newton
 */

#ifndef SANDNETPLUGIN_H
#define SANDNETPLUGIN_H

#include <lla/plugin.h>

class SandNetDevice ;

class SandNetPlugin : public Plugin {

	public:
		SandNetPlugin(PluginAdaptor *pa) : Plugin(pa) {m_enabled = false; }

		int start();
		int stop();
		int test(void *data);
		bool is_enabled() 	{ return m_enabled; }
		char *get_name() 	{ return "SandNet Plugin"; }
		char *get_desc() ;

	private:
		class Preferences *load_prefs() ;
		
		class Preferences *m_prefs ;
		SandNetDevice *m_dev ;		// only have one device
		bool m_enabled ;			// are we running
};

#endif

