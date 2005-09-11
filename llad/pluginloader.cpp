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
 * pluginloader.cpp
 * This class is responsible for loading and unloading the plugins
 * Copyright (C) 2005  Simon Newton
 */

#include "pluginloader.h" 

#include <lla/logger.h>

#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <stdio.h>

#define SHARED_LIB_EXT ".so"

/*
 * Create a new plugin loader
 *
 * @param pa	pointer to a pluginAdaptor object
 */
PluginLoader::PluginLoader(PluginAdaptor *pa) {
	this->pa = pa ;
}


/*
 * Destroy this object, this will disable and unload all plugins
 *
 */
PluginLoader::~PluginLoader() {
	int i;
	map<void*,Plugin*>::iterator iter;
	
	for(i=0; i < m_plugin_vect.size() ; i++) {
		// FIX: this better not fail ...
		m_plugin_vect[i]->stop() ;
	}

	//unload all plugins
	for(iter = m_plugin_map.begin(); iter != m_plugin_map.end(); iter++) {
		unload_plugin((*iter).first) ;
	}

	m_plugin_map.clear() ;
}


/*
 * Read the directory and load all .so files
 *
 * @param dirname	the plugins directory
 * @return 	0 on sucess, -1 on failure
 */
int PluginLoader::load_plugins(char *dirname) {
	int slen ;
	char *filename, *ext ;
	Plugin *plug = NULL ;
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;

	dir = opendir(dirname);
	
	if (!dir)
		return 0;

	while ((ent = readdir(dir)) != NULL) {

		// this needs to be cleaned up -> strdup_printf
		slen = strlen(dirname) + strlen(ent->d_name ) + 2 ;
		filename = (char *) malloc(slen) ;

		if(filename == NULL)
			break;

		strcpy(filename, dirname) ;
		strcat(filename, "/") ;
		strcat(filename, ent->d_name) ;
		
		if (!stat(filename, &statbuf) && S_ISREG(statbuf.st_mode) && (ext = strrchr(ent->d_name, '.')) != NULL)
		if (!strcmp(ext, SHARED_LIB_EXT)) {
				// ok try and load it
				if( (plug = this->load_plugin(filename)) == NULL) {
						Logger::instance()->log(Logger::WARN, "Failed to load plugin: %s", filename) ;
				} else {
					m_plugin_vect.push_back(plug) ;
				}
			}
		free(filename) ;
	}
	closedir(dir);

	return 0 ;
}


/*
 * Return the number of plugins loaded
 *
 * @return the number of plugins loaded
 */
int PluginLoader::plugin_count() {
	return m_plugin_vect.size() ;
}


/*
 * Return the plugin with the specified id
 *
 * @param id 	the id of the plugin to fetch
 * @return	the plugin with the specified id
 */
Plugin *PluginLoader::get_plugin(int id) {
	return m_plugin_vect[id] ;
}


// Private Functions
//-----------------------------------------------------------------------------

/*
 * Load a plugin from a file
 *
 * @param	path	the path to the plugin
 * @return 0 on sucess, -1 on failure
 */
Plugin *PluginLoader::load_plugin(char *path) {
	void* handle = NULL;
	Plugin *plug ;
	create_t *create ;
	
	if ( (handle = dlopen(path, RTLD_LAZY)) == NULL) {
		Logger::instance()->log(Logger::WARN, "dlopen: %s", dlerror()) ;
		return NULL ;
	}

	// reset dlerror
	dlerror() ;
	create = (create_t*) dlsym(handle, "create");

	if(dlerror() != NULL) {
		Logger::instance()->log(Logger::WARN, "Could not locate symbol") ;
		dlclose(handle) ;
		return NULL;
	}

	// init plugin
	if ( (plug = create(pa)) == NULL) {
		dlclose(handle) ;
		return NULL ;
	}
	
	pair<void*, Plugin*> p (handle, plug) ;
	m_plugin_map.insert(p) ;

	Logger::instance()->log(Logger::WARN, "Loaded plugin %s", plug->get_name()) ;
	
	return plug ;
}


/*
 * Unload the plugin
 *
 * @param handle	the handle of the plugin to unload
 * @return	0 on success, non 0 on failure
 */
int PluginLoader::unload_plugin(void *handle) {
	destroy_t *destroy ;

	// reset dlerror
	dlerror() ;
	destroy =  (destroy_t*) dlsym(handle, "destroy");

	if(dlerror() != NULL) {
		Logger::instance()->log(Logger::WARN, "Could not locate destory symbol") ;
		return -1;
	}

	destroy(m_plugin_map[handle]) ;
	dlclose(handle) ;

	return 0 ;
}
