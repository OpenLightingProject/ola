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
 * plugin.h
 * Header file for plugin class
 * Copyright (C) 2005 Simon Newton
 *
 *
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#include <lla/plugin_id.h>

class PluginAdaptor ;

/*
 * Represents a plugin
 *
 */
class Plugin {
	
	public :
		Plugin(const PluginAdaptor *pa, lla_plugin_id id) : m_pa(pa), m_id(id) {} 
		virtual ~Plugin() {};

		virtual const char *get_name() const = 0 ;
		virtual int start() = 0 ;
		virtual int stop() = 0 ;
		virtual bool is_enabled() const = 0;
		virtual const char *get_desc() const = 0;
		lla_plugin_id get_id() { return m_id;}

	protected:
		const PluginAdaptor *m_pa ;

	private:
		Plugin(const Plugin&);
		Plugin& operator=(const Plugin&);
		lla_plugin_id m_id;
	
	
} ;

// interface functions
typedef Plugin* create_t(const PluginAdaptor *pa);
typedef void destroy_t(Plugin*);

#endif
