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

#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>

#include <llad/logger.h>
#include <llad/Plugin.h>

#include "DlOpenPluginLoader.h"

namespace lla {

using namespace std;


/*
 * Read the directory and load all .so files
 *
 * @return   0 on sucess, -1 on failure
 */
int DlOpenPluginLoader::LoadPlugins() {
  AbstractPlugin *plugin = NULL;
  set<string>::iterator iter;
  set<string> plugin_names = FindPlugins(m_dirname);

  if (!m_dl_active) {
    if (lt_dlinit()) {
      Logger::instance()->log(Logger::CRIT, "lt_dlinit failed");
      return -1;
    }
  }

  if (lt_dlsetsearchpath(m_dirname.c_str())) {
    Logger::instance()->log(Logger::CRIT, "lt_setpath failed");
    lt_dlexit();
    return -1;
  }
  m_dl_active = true;

  for (iter = plugin_names.begin(); iter != plugin_names.end(); ++iter) {
    string path = m_dirname;
    path.append("/");
    path.append(*iter);

    if ((plugin = this->LoadPlugin(path)) == NULL) {
      Logger::instance()->log(Logger::WARN, "Failed to load plugin: %s", path.c_str());
    } else {
      m_plugins.push_back(plugin);
    }
  }

  return 0;
}


/*
 * Unload all plugins
 *
 */
int DlOpenPluginLoader::UnloadPlugins() {
  map<lt_dlhandle, AbstractPlugin*>::iterator map_iter;
  vector<AbstractPlugin*>::iterator iter;

  for (iter = m_plugins.begin(); iter != m_plugins.end(); ++iter) {
    if((*iter)->IsEnabled())
      (*iter)->Stop();
  }

  for (map_iter = m_plugin_map.begin(); map_iter != m_plugin_map.end(); map_iter++)
    UnloadPlugin((*map_iter).first);

  m_plugin_map.clear();
  m_plugins.clear();

  if (m_dl_active) {
    if (lt_dlexit()) {
      printf("dlexit: %s\n", (char *) lt_dlerror());
    }
    m_dl_active = false;
  }
  return 0;
}


/*
 * Return the number of plugins loaded
 *
 * @return the number of plugins loaded
 */
int DlOpenPluginLoader::PluginCount() const {
  return m_plugins.size();
}


/*
 * Return the plugin with the specified id
 *
 * @param id   the id of the plugin to fetch
 * @return  the plugin with the specified id
 */
AbstractPlugin *DlOpenPluginLoader::GetPlugin(unsigned int id) const {
  if (id > m_plugins.size() )
    return NULL;
  return m_plugins[id];
}


// Private Functions
//-----------------------------------------------------------------------------


/*
 * Find a list of possible plugins
 */
set<string> DlOpenPluginLoader::FindPlugins(const string &path) {
  DIR *dir;
  struct dirent *ent;
  set<string> plugin_names;

  dir = opendir(path.c_str());

  if (!dir) {
    Logger::instance()->log(Logger::WARN,
                            "Plugin directory %s doesn't exist",
                            path.c_str());
    return plugin_names;
  }

  // find files that could be plugins
  while ((ent = readdir(dir)) != NULL) {
    string fname = ent->d_name;
    string::size_type i = fname.find_first_of(".");
    if (i == string::npos || i == 0)
      continue;
    plugin_names.insert(fname.substr(0, i));
  }

  closedir(dir);
  return plugin_names;
}


/*
 * Load a plugin from a file
 *
 * @param  path  the path to the plugin
 * @return 0 on sucess, -1 on failure
 */
AbstractPlugin *DlOpenPluginLoader::LoadPlugin(const string &path) {
  lt_dlhandle module = NULL;
  AbstractPlugin *plugin;
  create_t *create;

  module = lt_dlopenext(path.c_str());

  if (!module) {
    printf("failed to dlopen\n");
    Logger::instance()->log(Logger::WARN, "lt_dlopen: %s", lt_dlerror());
    return NULL;
  }

  create = (create_t*) lt_dlsym(module, "create");

  if (lt_dlerror()) {
    Logger::instance()->log(Logger::WARN, "Could not locate symbol");
    lt_dlclose(module);
    return NULL;
  }

  // init plugin
  if ((plugin = create(m_plugin_adaptor)) == NULL) {
    lt_dlclose(module);
    return NULL;
  }

  pair<lt_dlhandle, AbstractPlugin*> pair (module, plugin);
  m_plugin_map.insert(pair);

  Logger::instance()->log(Logger::INFO, "Loaded plugin %s", plugin->Name().c_str());
  return plugin;
}


/*
 * Unload the plugin
 *
 * @param handle  the handle of the plugin to unload
 * @return  0 on success, non 0 on failure
 */
int DlOpenPluginLoader::UnloadPlugin(lt_dlhandle handle) {
  destroy_t *destroy = (destroy_t*) lt_dlsym(handle, "destroy");

  if (lt_dlerror() != NULL) {
    Logger::instance()->log(Logger::WARN, "Could not locate destroy symbol");
    return -1;
  }

  destroy(m_plugin_map[handle]);
  lt_dlclose(handle);
  return 0;
}

} //lla
