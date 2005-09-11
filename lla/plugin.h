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

class PluginAdaptor ;

/*
 * Represents a plugin
 *
 */
class Plugin {
	
	public :
		Plugin(PluginAdaptor *pa) { m_pa = pa; } 

		virtual char *get_name() = 0 ;
		virtual int start() = 0 ;
		virtual int stop() = 0 ;
		virtual bool is_enabled() = 0;
		virtual char *get_desc() = 0;

	protected:
		PluginAdaptor *m_pa ;
} ;

// interface functions
typedef Plugin* create_t(PluginAdaptor *pa);
typedef void destroy_t(Plugin*);

#endif
