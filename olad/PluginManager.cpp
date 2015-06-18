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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * PluginManager.cpp
 * This class is responsible for loading and unloading the plugins
 * Copyright (C) 2005 Simon Newton
 */

#include "olad/PluginManager.h"

#include <set>
#include <vector>
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/PluginLoader.h"

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

void PluginManager::LoadAll() {
  m_enabled_plugins.clear();

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
        delete plugin;
        continue;
      }

      if (!plugin->LoadPreferences()) {
        OLA_WARN << "Failed to load preferences for " << plugin->Name();
        continue;
      }

      if (!plugin->IsEnabled()) {
        OLA_INFO << "Skipping " << plugin->Name() << " because it was disabled";
        continue;
      }
      STLInsertIfNotPresent(&m_enabled_plugins, plugin->Id(), plugin);
    }
  }

  // The second pass checks for conflicts and starts each plugin
  PluginMap::iterator plugin_iter = m_enabled_plugins.begin();
  for (; plugin_iter != m_enabled_plugins.end(); ++plugin_iter) {
    StartIfSafe(plugin_iter->second);
  }
}

void PluginManager::UnloadAll() {
  PluginMap::iterator plugin_iter = m_loaded_plugins.begin();
  for (; plugin_iter != m_loaded_plugins.end(); ++plugin_iter) {
    plugin_iter->second->Stop();
  }
  m_loaded_plugins.clear();
  m_active_plugins.clear();
  m_enabled_plugins.clear();

  vector<PluginLoader*>::iterator iter = m_plugin_loaders.begin();
  for (; iter != m_plugin_loaders.end(); ++iter) {
    (*iter)->SetPluginAdaptor(NULL);
    (*iter)->UnloadPlugins();
  }
}

void PluginManager::Plugins(vector<AbstractPlugin*> *plugins) const {
  plugins->clear();
  STLValues(m_loaded_plugins, plugins);
}

void PluginManager::ActivePlugins(vector<AbstractPlugin*> *plugins) const {
  plugins->clear();
  STLValues(m_active_plugins, plugins);
}

void PluginManager::EnabledPlugins(vector<AbstractPlugin*> *plugins) const {
  plugins->clear();
  STLValues(m_enabled_plugins, plugins);
}

AbstractPlugin* PluginManager::GetPlugin(ola_plugin_id plugin_id) const {
  return STLFindOrNull(m_loaded_plugins, plugin_id);
}

bool PluginManager::IsActive(ola_plugin_id plugin_id) const {
  return STLContains(m_active_plugins, plugin_id);
}

bool PluginManager::IsEnabled(ola_plugin_id plugin_id) const {
  return STLContains(m_enabled_plugins, plugin_id);
}

void PluginManager::DisableAndStopPlugin(ola_plugin_id plugin_id) {
  AbstractPlugin *plugin = STLFindOrNull(m_loaded_plugins, plugin_id);
  if (!plugin_id) {
    return;
  }

  if (STLRemove(&m_active_plugins, plugin_id)) {
    plugin->Stop();
  }

  if (STLRemove(&m_enabled_plugins, plugin_id)) {
    plugin->SetEnabledState(false);
  }
}

bool PluginManager::EnableAndStartPlugin(ola_plugin_id plugin_id) {
  if (STLContains(m_active_plugins, plugin_id)) {
    // Already running, nothing to do.
    return true;
  }

  AbstractPlugin *plugin = STLFindOrNull(m_loaded_plugins, plugin_id);
  if (!plugin) {
    return false;
  }

  if (STLInsertIfNotPresent(&m_enabled_plugins, plugin_id, plugin)) {
    plugin->SetEnabledState(true);
  }

  return StartIfSafe(plugin);
}

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
        if (plugin) {
          plugins->push_back(plugin);
        }
      }
    } else {
      if (STLContains(conflict_list, plugin_id)) {
        plugins->push_back(iter->second);
      }
    }
  }
}

bool PluginManager::StartIfSafe(AbstractPlugin *plugin) {
  AbstractPlugin *conflicting_plugin = CheckForRunningConflicts(plugin);
  if (conflicting_plugin) {
    OLA_WARN << "Not enabling " << plugin->Name()
             << " because it conflicts with "
             << conflicting_plugin->Name()
             << " which is already running";
    return false;
  }

  OLA_INFO << "Trying to start " << plugin->Name();
  bool ok = plugin->Start();
  if (!ok) {
    OLA_WARN << "Failed to start " << plugin->Name();
  } else {
    OLA_INFO << "Started " << plugin->Name();
    STLReplace(&m_active_plugins, plugin->Id(), plugin);
  }
  return ok;
}

/*
 * @brief Check if this plugin conflicts with any of the running plugins.
 * @param plugin The plugin to check
 * @returns The first conflicting plugin, or NULL if there aren't any.
 */
AbstractPlugin* PluginManager::CheckForRunningConflicts(
    const AbstractPlugin *plugin) const {
  PluginMap::const_iterator iter = m_active_plugins.begin();
  for (; iter != m_active_plugins.end(); ++iter) {
    set<ola_plugin_id> conflict_list;
    iter->second->ConflictsWith(&conflict_list);
    if (STLContains(conflict_list, plugin->Id())) {
      return iter->second;
    }
  }

  set<ola_plugin_id> conflict_list;
  plugin->ConflictsWith(&conflict_list);
  set<ola_plugin_id>::const_iterator set_iter = conflict_list.begin();
  for (; set_iter != conflict_list.end(); ++set_iter) {
    AbstractPlugin *conflicting_plugin = STLFindOrNull(
        m_active_plugins, *set_iter);
    if (conflicting_plugin) {
      return conflicting_plugin;
    }
  }
  return NULL;
}
}  // namespace ola
