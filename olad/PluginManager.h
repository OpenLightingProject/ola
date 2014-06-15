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
 * PluginManager.h
 * Interface to the PluginManager class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_PLUGINMANAGER_H_
#define OLAD_PLUGINMANAGER_H_

#include <map>
#include <vector>

#include "ola/base/Macro.h"

namespace ola {

class PluginLoader;
class PluginAdaptor;
class AbstractPlugin;

class PluginManager {
 public:
    PluginManager(const std::vector<PluginLoader*> &plugin_loaders,
                  PluginAdaptor *plugin_adaptor);
    ~PluginManager();

    void LoadAll();
    void UnloadAll();

    // Return a list of all loaded plugins, this includes active and inactive
    // plugins.
    void Plugins(std::vector<AbstractPlugin*> *plugins) const;

    // Return a list of all plugins that are active. Note that even though a
    // plugin may be enabled, it may not be active due to conflicts with
    // other plugins.
    void ActivePlugins(std::vector<AbstractPlugin*> *plugins) const;

    // Lookup a plugin by ID
    AbstractPlugin* GetPlugin(ola_plugin_id plugin_id) const;

    // Returns if a plugin is active.
    bool IsActive(ola_plugin_id plugin_id) const;

    // Return a list of plugins that conflict with this plugin
    void GetConflictList(ola_plugin_id plugin_id,
                         std::vector<AbstractPlugin*> *plugins);

 private:
    typedef std::map<ola_plugin_id, AbstractPlugin*> PluginMap;

    std::vector<PluginLoader*> m_plugin_loaders;
    PluginMap m_loaded_plugins;  // plugins that are loaded
    PluginMap m_active_plugins;  // active plugins
    PluginAdaptor *m_plugin_adaptor;

    DISALLOW_COPY_AND_ASSIGN(PluginManager);
};
}  // namespace ola
#endif  // OLAD_PLUGINMANAGER_H_
