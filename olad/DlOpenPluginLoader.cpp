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
#include <set>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "olad/Plugin.h"
#include "olad/DlOpenPluginLoader.h"

namespace ola {


/*
 * Read the directory and load all .so files
 */
vector<AbstractPlugin*> DlOpenPluginLoader::LoadPlugins() {
  AbstractPlugin *plugin = NULL;
  set<string>::iterator iter;
  set<string> plugin_names = FindPlugins(m_dirname);
  vector<AbstractPlugin*> plugins;

  if (!m_dl_active) {
    if (lt_dlinit()) {
      OLA_WARN << "lt_dlinit failed";
      return plugins;
    }
  }

  if (lt_dlsetsearchpath(m_dirname.c_str())) {
    OLA_WARN << "lt_setpath failed";
    lt_dlexit();
    return plugins;
  }
  m_dl_active = true;

  for (iter = plugin_names.begin(); iter != plugin_names.end(); ++iter) {
    string path = m_dirname;
    path.append("/");
    path.append(*iter);

    if ((plugin = this->LoadPlugin(path)))
      plugins.push_back(plugin);
    else
      OLA_WARN << "Failed to load plugin: " << path;
  }
  return plugins;
}


/*
 * Unload all plugins
 */
void DlOpenPluginLoader::UnloadPlugins() {
  vector<lt_dlhandle>::iterator iter;

  for (iter = m_plugin_handles.begin(); iter != m_plugin_handles.end(); iter++)
    lt_dlclose(*iter);
  m_plugin_handles.clear();

  if (m_dl_active) {
    if (lt_dlexit())
      OLA_WARN << "dlexit: " << lt_dlerror();
    m_dl_active = false;
  }
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
    OLA_WARN << "Plugin directory " << path << " doesn't exist";
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
 * @param  path  the path to the plugin
 * @returns the plugin or NULL
 */
AbstractPlugin *DlOpenPluginLoader::LoadPlugin(const string &path) {
  lt_dlhandle module = NULL;
  AbstractPlugin *plugin;
  create_t *create;

  OLA_INFO << "Attempting to load " << path;
  module = lt_dlopenext(path.c_str());

  if (!module) {
    OLA_WARN << "failed to lt_dlopen " << path << ": " << lt_dlerror();
    return NULL;
  }

  create = reinterpret_cast<create_t*>(lt_dlsym(module, "create"));

  if (lt_dlerror()) {
    OLA_WARN << "Could not locate create symbol in " << path;
    lt_dlclose(module);
    return NULL;
  }

  // init plugin
  if ((plugin = create(m_plugin_adaptor)) == NULL) {
    lt_dlclose(module);
    return NULL;
  }

  m_plugin_handles.push_back(module);
  OLA_INFO << "Loaded plugin " << plugin->Name();
  return plugin;
}
}  // ola
