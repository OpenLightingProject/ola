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
 * preferences.cpp
 * This class is responsible for storing preferences
 * Copyright (C) 2005  Simon Newton
 */

#include <llad/preferences.h>
#include <llad/logger.h>

#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include <list>

#define LLA_CONFIG_DIR ".lla"
#define LLA_CONFIG_PREFIX "lla-"
#define LLA_CONFIG_SUFFIX ".conf"

/*
 * Create a new preference container
 *
 * @param id	the id of the container
 */
Preferences::Preferences(string id) : id(id) {}


/*
 * Destroy this object
 *
 */
Preferences::~Preferences() {
	m_pref_map.clear() ;
}


/*
 * Load the preferences from storage
 *
 */
int Preferences::load() {
	#define BUF_SIZE 1024
	FILE *fh ;
	string filename, line ;
	char buf[BUF_SIZE], *k_c, *v_c ;
	string key, val ;
	
	if(change_dir())
		return -1 ;

	filename = LLA_CONFIG_PREFIX + id + LLA_CONFIG_SUFFIX ;
	if ((fh = fopen(filename.c_str(), "r")) == NULL ) {
		Logger::instance()->log(Logger::INFO, "Failed to open %s:  %s", filename.c_str(), strerror(errno)) ;
		return -1 ;
	}

	while (  fgets(buf, 1024, fh) != NULL) {
		if(*buf == '#')
			continue ;

		k_c = strtok(buf, "=") ;
		v_c = strtok(NULL, "=") ;
		
		if(k_c == NULL || v_c == NULL)
			continue ;
		
		key = strtrim(k_c);
		val = strtrim(v_c);

		m_pref_map.insert(pair<string,string>(key,val)) ;
		
	}

	fclose(fh) ;
	return 0 ;
}


/*
 * Save the preferences to storage
 *
 *
 */
int Preferences::save() const {
    map<string, string>::const_iterator iter;
	string filename ;
	FILE *fh ;

	if(change_dir())
		return -1 ;
	
	filename = LLA_CONFIG_PREFIX + id + LLA_CONFIG_SUFFIX ;
	if ((fh = fopen(filename.c_str(), "w")) == NULL ) {
		perror("fopen") ;
		return -1 ;
	}
	
    for (iter=m_pref_map.begin(); iter != m_pref_map.end(); ++iter) {
		fprintf(fh, "%s = %s\n", iter->first.c_str() , iter->second.c_str() ) ;
    }
	
	fclose(fh) ;

	return 0 ;
}


/*
 * Set a preference value, overiding the existing value.
 *
 * @param key
 * @param value
 */
int Preferences::set_val(const string &key, const string &val) {

	m_pref_map.erase(key);

	m_pref_map.insert(pair<string,string>(key,val)) ;
	return 0;	
}


/*
 * Adds this preference value to the store
 *
 * @param key
 * @param value
 */
int Preferences::set_multiple_val(const string &key, const string &val) {
	m_pref_map.insert(pair<string,string>(key,val)) ;
	return 0;	
}


/*
 * Get a preference value
 *
 * @param key
 * @return the value corrosponding to key
 *
 */
string Preferences::get_val(const string &key) {
    map<string, string>::const_iterator iter;

	iter = m_pref_map.find(key) ;

	if( iter != m_pref_map.end() ) {
		return iter->second.c_str() ;
	}
	
	return "" ;
}


/*
 * returns all preference values corrosponding to this key
 *
 * the vector must be freed once used.
 */
vector<string> *Preferences::get_multiple_val(const string &key) {
	vector<string> *vec = new vector<string> ;
    map<string, string>::const_iterator iter;

	if(vec == NULL) 
		return NULL ;

	for ( iter = m_pref_map.find(key) ;  iter != m_pref_map.end() && iter->first == key ; ++iter ) {
		vec->push_back(iter->second) ;
	}
	return vec ;

}


//-----------------------------------------------------------------------------
// Private functions follow


int Preferences::change_dir() const {
	struct passwd  *ptr = getpwuid(getuid()) ;
	if(ptr == NULL) 
		return -1 ;

	if(chdir(ptr->pw_dir))
		return -1 ;

	if(chdir(LLA_CONFIG_DIR)) {
		// try and create it
		if(mkdir(LLA_CONFIG_DIR, 0777)) 
			return -1 ;

		if(chdir(LLA_CONFIG_DIR))
			return -1 ;
	}
	return 0 ;
}


/*
 * trim leading and trailing whitespace from the string
 *
 * @param str	pointer to the string
 *
 *
 */
char *Preferences::strtrim(char *str) {
	int n = strlen(str) ;
	char *beg, *end;
	beg = str;
	end = str + (n-1); /* Point to the last non-null character in the string */
	
	while(*beg == ' ' || *beg == '\t') {
		beg++;
	}

	/* Remove trailing whitespace and null-terminate */
	while(*end == ' ' || *end == '\n' || *end == '\r' || *beg == '\t' ) {
		end--;
	}

	*(end+1) = '\0';
	/* Shift the string */
	memmove(str, beg, (end - beg + 2));
	return str;
}
