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
 * preferences.h
 * Interface for the preferences class
 * Copyright (C) 2005  Simon Newton
 */

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <map>

using namespace std;

class Preferences {

	public:
		Preferences(const char *id) ;
		~Preferences() ;

		int load() ;
		int save() ;

		int set_val(const char *key, const char *value) ;
		char *get_val(const char *key) ;
		
	private:
		
		map<char*, char*> m_pref_map ;

};

#endif
