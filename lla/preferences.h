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
 * Copyright (C) 2005-2006  Simon Newton
 */

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <map>
#include <vector>
#include <string>

using namespace std;

class Preferences {

	public:
		Preferences(string id) ;
		~Preferences() ;

		int load() ;
		int save() ;

		int set_val(string key, string value) ;
		int set_single_val(string key, string value) ;
		
		string get_val(string key) ;
		vector<string> *get_multiple_val(string key) ;
		
	private:
		int change_dir() ;
		char *strtrim(char *str) ;

		string id ;
		multimap<string, string> m_pref_map ;

};

#endif
