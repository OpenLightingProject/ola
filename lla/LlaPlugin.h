/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * LlaPlugin.h
 * Interface to the LLA Client Plugin class
 * Copyright (C) 2005-2006 Simon Newton
 */

//#include <lla/plugin_id.h>

#ifndef LLAPLUGIN_H
#define LLAPLUGIN_H

using namespace std;

#include <string>

class LlaPlugin {

	public:
		LlaPlugin(int id, const string &name) : m_id(id), m_name(name) {};
		~LlaPlugin() {};

		int get_id() { return m_id; }
		string get_name() { return m_name; }
		string get_desc() { return m_desc; }
		void set_desc(const string &desc) { m_desc = desc; }
	private:
		LlaPlugin(const LlaPlugin&);
		LlaPlugin operator=(const LlaPlugin&);

		int m_id;		// id of this plugin
		string m_name;	// plugin name
		string m_desc;

};
#endif
