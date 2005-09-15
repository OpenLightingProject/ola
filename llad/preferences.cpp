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

#include <lla/preferences.h> 
#include <lla/logger.h>

#include <string.h>
#include <dirent.h>
#include <stdio.h>


/*
 * Create a new preference container
 *
 * @param id	the id of the container
 */
Preferences::Preferences(const char *id) {



}


/*
 * Destroy this object, this will disable and unload all plugins
 *
 */
Preferences::~Preferences() {

	m_pref_map.clear() ;
}


/*
 * Load from storage
 *
 */
int Preferences::load() {
/*
	#define BUF_SIZE 1024
	FILE *fh ;
	char buf[1024], *c ;
	char *key, *data ;
	
	if(ops->config_file == NULL)
		return -1;

	if ((fh = fopen(ops->config_file, "r")) == NULL ) {
		perror("fopen") ;
		return -1 ;
	}

	while (  fgets(buf, 1024, fh) != NULL) {
		if(*buf == '#')
			continue ;

		// strip \n
		for(c = buf ; *c != '\n' ; c++) ;
			*c = '\0' ;

		key = strtok(buf, "=") ;
		data = strtok(NULL, "=") ;

		if(key == NULL || data == NULL)
			continue ;

		if(strcmp(key, "Shortname") == 0) {
			free(ops->short_name) ;
			ops->short_name = strdup(data) ;
		} else if(strcmp(key, "Longname") == 0) {
			free(ops->long_name) ;
			ops->long_name = strdup(data) ;
		} else if(strcmp(key, "Subnet") == 0) {
			ops->subnet_addr = atoi(data) ;
		} else if(strcmp(key, "Port") == 0 ) {
			ops->port_addr = atoi(data) ;
		} 

	}

	fclose(fh) ;
	return 0 ;
*/
}


/*
 * save to storage
 *
 *
 */
int Preferences::save() {
/*	FILE *fh ;
	
	printf("in save config\n") ;
	if ((fh = fopen(ops->config_file, "w")) == NULL ) {
		perror("fopen") ;
		return -1 ;
	}

	fprintf(fh, "# artnet_usb config file\n") ;

	fclose(fh) ;

	return 0 ;
*/
}


/*
 * Set a preference value
 *
 * @param key
 * @param value
 */
int Preferences::set_val(const char *key, const char *val) {
	char *skey = strdup(key) ;
	char *sval = strdup(val) ;
	
	m_pref_map[skey] = sval ;
	

}


/*
 * Get a preference value
 *
 * @param key
 * @return the value corrosponding to key
 *
 */
char *Preferences::get_val(const char *key) {
	int i ;

	map<char *, char *>::iterator iter = m_pref_map.find((char*)key);
	
	if (iter != m_pref_map.end()) {
	   return iter->second ;
   	}
	
	return NULL ;
}
