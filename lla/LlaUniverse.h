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
 * LlaUniverse.h
 * Interface to the LLA Client Universe class
 * Copyright (C) 2005-2006 Simon Newton
 */

#ifndef LLAUNIVERSE_H
#define LLAUNIVERSE_H

using namespace std;

#include <string>

class LlaUniverse {

	public:
		LlaUniverse(int id, const string &name) : m_id(id), m_name(name) {};
		~LlaUniverse() {};

		int get_id() { return m_id;}
		string get_name() { return m_name;}

	private:
		LlaUniverse(const LlaUniverse&);
		LlaUniverse operator=(const LlaUniverse&);

		int m_id;		// id of this universe
		string m_name;	// universe name

};
#endif
