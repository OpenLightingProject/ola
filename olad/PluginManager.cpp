/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * PluginManager.cpp
 * This class is responsible for loading and unloading the plugins
 * Copyright (C) 2005-2007 Simon Newton
 */

#include <set>
#include <vector>
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/PluginLoader.h"
#include "olad/PluginManager.h"

namespace ola {

using std::vector;
using std::set;

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
  vector<AbstractPlugin*> enabled_plugins;
  set<ola_plugin_id> enabled_plugin_ids;

  // The first pass populates the m_plugin map, and builds a list of enabled
  // plugins.
  vector<PluginLoader*>::iterator iter;
  for (iter = m_plugin_loaders.begin(); iter != m_plugin_loaders.end();
       ++iter) {
    (*iter)->SetPluginAdaptor(m_plugin_adaptor);
    vector<AbstractPlugin*> plugins = (*iter)->LoadPlugins();

    vector<AbstractPlugin*>::iterator plugin_iter = plugins.begin();
    for (; plugin_iter != plugins.end(); ++plugin_iter) {
      AbstractPlugin *plugin = *plugin_iter;
      if (!STLInsertIfNotPresent(&m_loaded_plugins, plugin->Id(), plugin)) {
        OLA_WARN << "Skipping plugin " << plugin->Name()
                 << " because it's already been loaded";
        continue;
      }

      if (!plugin->LoadPreferences()) {
        OLA_WARN << "Failed to load prefernes for " << plugin->Name();
        continue;
      }

      if (!plugin->IsEnabled()) {
        OLA_INFO << "Skipping " << plugin->Name() << " because it was disabled";
        continue;
      }
      enabled_plugins.push_back(plugin);
      enabled_plugin_ids.insert(plugin->Id());
    }
  }

  // The second pass checks for conflicts and starts each plugin
  vector<AbstractPlugin*>::iterator plugin_iter = enabled_plugins.begin();
  for (; plugin_iter != enabled_plugins.end(); ++plugin_iter) {
    AbstractPlugin *plugin = *plugin_iter;

    // check for conflicts
    bool conflict = false;
    set<ola_plugin_id> conflict_list;
    plugin->ConflictsWith(&conflict_list);
    set<ola_plugin_id>::const_iterator set_iter = conflict_list.begin();
    for (; set_iter != conflict_list.end(); ++set_iter) {
      if (STLContains(enabled_plugin_ids, *set_iter)) {
        OLA_WARN << "Skipping " << plugin->Name() <<
          " because it conflicts with " << GetPlugin(*set_iter)->Name() <<
          " which is also enabled";
        conflict = true;
        break;
      }
    }

    if (conflict)
      continue;

    OLA_INFO << "Trying to start " << plugin->Name();
    if (!plugin->Start())
      OLA_WARN << "Failed to start " << plugin->Name();
    else
      OLA_INFO << "Started " << plugin->Name();
    STLReplace(&m_active_plugins, plugin->Id(), plugin);
  }
}


/*
 * Unload all the plugins.
 */
void PluginManager::UnloadAll() {
  PluginMap::iterator plugin_iter = m_loaded_plugins.begin();
  for (; plugin_iter != m_loaded_plugins.end(); ++plugin_iter) {
    plugin_iter->second->Stop();
  }
  m_loaded_plugins.clear();
  m_active_plugins.clear();

  vector<PluginLoader*>::iterator iter = m_plugin_loaders.begin();
  for (; iter != m_plugin_loaders.end(); ++iter) {
    (*iter)->SetPluginAdaptor(NULL);
    (*iter)->UnloadPlugins();
  }
}


/*
 * Return the list of plugins loaded
 */
void PluginManager::Plugins(vector<AbstractPlugin*> *plugins) const {
  plugins->clear();
  STLValues(m_loaded_plugins, plugins);
}


/*
 * Return the list of active plugins.
 */
void PluginManager::ActivePlugins(vector<AbstractPlugin*> *plugins) const {
  plugins->clear();
  STLValues(m_active_plugins, plugins);
}


/*
 * Lookup a plugin by ID.
 * @param plugin_id the id of the plugin to find
 * @return the plugin matching the id or NULL if not found.
 */
AbstractPlugin* PluginManager::GetPlugin(ola_plugin_id plugin_id) const {
  return STLFindOrNull(m_loaded_plugins, plugin_id);
}

/*
 * Returns true if the plugin was loaded.
 * @param plugin_id the id of the plugin to find
 */
bool PluginManager::IsActive(ola_plugin_id plugin_id) const {
  return STLContains(m_active_plugins, plugin_id);
}


/**
 * Return a list of plugins that conflict with this particular plugin.
 */
void PluginManager::GetConflictList(ola_plugin_id plugin_id,
                                    vector<AbstractPlugin*> *plugins) {
  PluginMap::iterator iter = m_loaded_plugins.begin();
  for (; iter != m_loaded_plugins.end(); ++iter) {
    set<ola_plugin_id> conflict_list;
    iter->second->ConflictsWith(&conflict_list);
    if (iter->second->Id() == plugin_id) {
      set<ola_plugin_id>::const_iterator id_iter = conflict_list.begin();
      for (; id_iter != conflict_list.end(); ++id_iter) {
        AbstractPlugin *plugin = GetPlugin(*id_iter);
        if (plugin)
          plugins->push_back(plugin);
      }
    } else {
      if (STLContains(conflict_list, plugin_id))
        plugins->push_back(iter->second);
    }
  }
}
}  // ola
