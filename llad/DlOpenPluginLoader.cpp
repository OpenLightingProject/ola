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
 * DlOpenPluginLoader.cpp
 * This plugin loader dynamically loads the plugins from a directory
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <llad/logger.h>

#include "DlOpenPluginLoader.h"

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
 * Read the directory and load all .so files
 *
 * @return   0 on sucess, -1 on failure
 */
int DlOpenPluginLoader::load_plugins() {
  Plugin *plug = NULL;
  DIR *dir;
  struct dirent *ent;
  struct stat statbuf;

  dir = opendir(m_dirname.c_str());

  if (!dir)
    return 0;

  while ((ent = readdir(dir)) != NULL) {
    string fname = m_dirname;
    fname.append("/");
    fname.append(ent->d_name);

    string::size_type i = fname.find_last_of(".");
    if ( i == string::npos)
      continue;

    if (!stat(fname.c_str(), &statbuf) && S_ISREG(statbuf.st_mode) && fname.substr(i) == SHARED_LIB_EXT) {

      // ok try and load it
      if( (plug = this->load_plugin(fname)) == NULL) {
        Logger::instance()->log(Logger::WARN, "Failed to load plugin: %s", fname.c_str());
      } else {
        m_plugin_vect.push_back(plug);
      }
    }
  }
  closedir(dir);

  return 0;
}


/*
 * unload all plugins
 *
 */
int DlOpenPluginLoader::unload_plugins() {
  unsigned int i;
  map<void*,Plugin*>::iterator iter;
  for(i=0; i < m_plugin_vect.size(); i++) {
    // TODO: this better not fail ...
    if( m_plugin_vect[i]->is_enabled()) {
      m_plugin_vect[i]->stop();
    }
  }

  //unload all plugins
  for(iter = m_plugin_map.begin(); iter != m_plugin_map.end(); iter++) {
    unload_plugin((*iter).first);
  }

  m_plugin_map.clear();
  m_plugin_vect.clear();

  return 0;
}


/*
 * Return the number of plugins loaded
 *
 * @return the number of plugins loaded
 */
int DlOpenPluginLoader::plugin_count() const {
  return m_plugin_vect.size();
}


/*
 * Return the plugin with the specified id
 *
 * @param id   the id of the plugin to fetch
 * @return  the plugin with the specified id
 */
Plugin *DlOpenPluginLoader::get_plugin(unsigned int id) const {

  if ( id > m_plugin_vect.size() )
    return NULL;

  return m_plugin_vect[id];
}


// Private Functions
//-----------------------------------------------------------------------------

/*
 * Load a plugin from a file
 *
 * @param  path  the path to the plugin
 * @return 0 on sucess, -1 on failure
 */
Plugin *DlOpenPluginLoader::load_plugin(const string &path) {
  void* handle = NULL;
  Plugin *plug;
  create_t *create;

  if ( (handle = dlopen(path.c_str(), RTLD_LAZY)) == NULL) {
    Logger::instance()->log(Logger::WARN, "dlopen: %s", dlerror());
    return NULL;
  }

  // reset dlerror
  dlerror();
  create = (create_t*) dlsym(handle, "create");

  if(dlerror() != NULL) {
    Logger::instance()->log(Logger::WARN, "Could not locate symbol");
    dlclose(handle);
    return NULL;
  }

  // init plugin
  if ( (plug = create(m_pa)) == NULL) {
    dlclose(handle);
    return NULL;
  }

  pair<void*, Plugin*> p (handle, plug);
  m_plugin_map.insert(p);

  Logger::instance()->log(Logger::WARN, "Loaded plugin %s", plug->get_name().c_str());

  return plug;
}


/*
 * Unload the plugin
 *
 * @param handle  the handle of the plugin to unload
 * @return  0 on success, non 0 on failure
 */
int DlOpenPluginLoader::unload_plugin(void *handle) {
  destroy_t *destroy;

  // reset dlerror
  dlerror();
  destroy =  (destroy_t*) dlsym(handle, "destroy");

  if(dlerror() != NULL) {
    Logger::instance()->log(Logger::WARN, "Could not locate destroy symbol");
    return -1;
  }

  destroy(m_plugin_map[handle]);
  dlclose(handle);

  return 0;
}
