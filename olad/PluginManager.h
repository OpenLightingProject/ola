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
#include "ola/plugin_id.h"

namespace ola {

class PluginLoader;
class PluginAdaptor;
class AbstractPlugin;

/**
 * @brief The manager of plugins.
 *
 * The plugin manager is responsible for loading the plugins (via
 * PluginLoaders) and retains ownership of the Plugin objects.
 *
 * Each plugin has a numeric ID associated with it. The plugin IDs can be found
 * in common/protocol/Ola.proto
 *
 * Plugins can be disabled through the preferences file. Some plugins may
 * conflict with others, in this case the first plugin will be started and the
 * rest of the conflicting plugins are ignored.
 *
 * Plugins are active if they weren't disabled, there were no conflicts that
 * prevented them from loading, and the call to Start() was successful.
 */
class PluginManager {
 public:
  /**
   * @brief Create a new PluginManager.
   * @param plugin_loaders the list of PluginLoader to use.
   * @param plugin_adaptor the PluginAdaptor to pass to each plugin.
   */
  PluginManager(const std::vector<PluginLoader*> &plugin_loaders,
                PluginAdaptor *plugin_adaptor);

  /**
   * @brief Destructor.
   */
  ~PluginManager();

  /**
   * @brief Attempt to load all the plugins and start them.
   *
   * Some plugins may not be started due to conflicts or being disabled.
   */
  void LoadAll();

  /**
   * Unload all the plugins.
   */
  void UnloadAll();

  /**
   * @brief Return the list of loaded plugins.
   * @param[out] plugins the list of plugins.
   *
   * This list includes disabled and conflicting plugins.
   */
  void Plugins(std::vector<AbstractPlugin*> *plugins) const;

  /**
   * @brief Return a list of active plugins.
   * @param[out] plugins the list of active plugins.
   */
  void ActivePlugins(std::vector<AbstractPlugin*> *plugins) const;

  /**
   * @brief Return a list of enabled plugins.
   * @param[out] plugins the list of enabled plugins.
   */
  void EnabledPlugins(std::vector<AbstractPlugin*> *plugins) const;

  /**
   * @brief Lookup a plugin by ID.
   * @param plugin_id the id of the plugin to find.
   * @return the plugin matching the id or NULL if not found.
   */
  AbstractPlugin* GetPlugin(ola_plugin_id plugin_id) const;

  /**
   * @brief Check if a plugin is active.
   * @param plugin_id the id of the plugin to check.
   * @returns true if the plugin is active, false otherwise.
   */
  bool IsActive(ola_plugin_id plugin_id) const;

  /**
   * @brief Check if a plugin is enabled.
   * @param plugin_id the id of the plugin to check.
   * @returns true if the plugin is enabled, false otherwise.
   */
  bool IsEnabled(ola_plugin_id plugin_id) const;

  /**
   * @brief Enable & start a plugin
   * @param plugin_id the id of the plugin to start.
   * @returns true if the plugin was started or was already running, false if
   * it couldn't be started.
   *
   * This call will enable a plugin, but may not start it due to conflicts with
   * existing plugins.
   */
  bool EnableAndStartPlugin(ola_plugin_id plugin_id);

  /**
   * @brief Disable & stop a plugin.
   * @param plugin_id the id of the plugin to stop.
   */
  void DisableAndStopPlugin(ola_plugin_id plugin_id);

  /**
   * @brief Return a list of plugins that conflict with this particular plugin.
   * @param plugin_id the id of the plugin to check.
   * @param[out] plugins the list of plugins that conflict with this one.
   */
  void GetConflictList(ola_plugin_id plugin_id,
                       std::vector<AbstractPlugin*> *plugins);

 private:
  typedef std::map<ola_plugin_id, AbstractPlugin*> PluginMap;

  std::vector<PluginLoader*> m_plugin_loaders;
  PluginMap m_loaded_plugins;  // plugins that are loaded
  PluginMap m_active_plugins;  // active plugins
  PluginMap m_enabled_plugins;  // enabled plugins
  PluginAdaptor *m_plugin_adaptor;

  bool StartIfSafe(AbstractPlugin *plugin);
  AbstractPlugin* CheckForRunningConflicts(const AbstractPlugin *plugin) const;

  DISALLOW_COPY_AND_ASSIGN(PluginManager);
};
}  // namespace ola
#endif  // OLAD_PLUGINMANAGER_H_
