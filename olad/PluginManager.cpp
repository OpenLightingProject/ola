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
 * PluginManager.cpp
 * This class is responsible for loading and unloading the plugins
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <vector>
#include "ola/Logging.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/PluginLoader.h"
#include "olad/PluginManager.h"

namespace ola {

using std::vector;

PluginManager::PluginManager(const vector<PluginLoader*> &plugin_loaders,
                             class PluginAdaptor *plugin_adaptor)
    : m_plugin_loaders(plugin_loaders),
      m_plugin_adaptor(plugin_adaptor) {
}


PluginManager::~PluginManager() {
  UnloadAll();
}


/*
 * Load all the plugins and start them.
 */
void PluginManager::LoadAll() {
  vector<PluginLoader*>::iterator iter;
  vector<AbstractPlugin*>::iterator plugin_iter;

  for (iter = m_plugin_loaders.begin(); iter != m_plugin_loaders.end();
       ++iter) {
    (*iter)->SetPluginAdaptor(m_plugin_adaptor);
    vector<AbstractPlugin*> plugins = (*iter)->LoadPlugins();

    for (plugin_iter = plugins.begin(); plugin_iter != plugins.end();
         ++plugin_iter) {
      if (GetPlugin((*plugin_iter)->Id()))
        OLA_WARN << "Skipping plugin " << (*plugin_iter)->Name() <<
          " because it's already been loaded";
      else
        m_plugins.push_back(*plugin_iter);
    }
  }

  for (plugin_iter = m_plugins.begin(); plugin_iter != m_plugins.end();
       ++plugin_iter) {
    OLA_INFO << "Trying to start " << (*plugin_iter)->Name();
    if (!(*plugin_iter)->Start())
      OLA_WARN << "Failed to start " << (*plugin_iter)->Name();
    else
      OLA_INFO << "Started " << (*plugin_iter)->Name();
  }
}


/*
 * Unload all the plugins.
 */
void PluginManager::UnloadAll() {
  vector<PluginLoader*>::iterator iter;
  vector<AbstractPlugin*>::iterator plugin_iter;

  for (plugin_iter = m_plugins.begin(); plugin_iter != m_plugins.end();
       ++plugin_iter) {
    if ((*plugin_iter)->IsEnabled())
      (*plugin_iter)->Stop();
    delete *plugin_iter;
  }
  m_plugins.clear();

  for (iter = m_plugin_loaders.begin(); iter != m_plugin_loaders.end();
       ++iter) {
    (*iter)->SetPluginAdaptor(NULL);
    (*iter)->UnloadPlugins();
  }
}


/*
 * Return the list of plugins loaded
 */
void PluginManager::Plugins(vector<AbstractPlugin*> *plugins) const {
  *plugins = m_plugins;
}


/*
 * Get a particular plugin
 * @param plugin_id the id of the plugin to find
 * @return the plugin matching the id or NULL if not found.
 */
AbstractPlugin* PluginManager::GetPlugin(ola_plugin_id plugin_id) const {
  vector<AbstractPlugin*>::const_iterator iter;

  for (iter = m_plugins.begin(); iter != m_plugins.end(); ++iter)
    if ((*iter)->Id() == plugin_id)
      return *iter;
  return NULL;
}
}  // ola
