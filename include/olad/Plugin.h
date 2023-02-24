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
 * Plugin.h
 * Header file for plugin class - plugins inherit from this.
 * Copyright (C) 2005 Simon Newton
 */

#ifndef INCLUDE_OLAD_PLUGIN_H_
#define INCLUDE_OLAD_PLUGIN_H_

#include <ola/base/Macro.h>
#include <ola/plugin_id.h>

#include <set>
#include <string>
#include <functional>

namespace ola {

class PluginAdaptor;

/**
 * The interface for a plugin
 */
class AbstractPlugin {
 public :
  AbstractPlugin() {}
  virtual ~AbstractPlugin() {}

  /**
   * @brief Load the preferences for a plugin and set defaults
   */
  virtual bool LoadPreferences() = 0;

  /**
   * @brief The location for preferences.
   *
   * This can be anything really but should indicate to the user how the
   * preferences were loaded.
   */
  virtual std::string PreferenceConfigLocation() const = 0;

  /**
   * @brief Is the plugin enabled?
   * @return true if this plugin is enabled
   */
  virtual bool IsEnabled() const = 0;

  /**
   * @brief Set the plugin's enabled state.
   * @param enable The new enabled state
   * @return true if this plugin is enabled
   */
  virtual void SetEnabledState(bool enable) = 0;

  /**
   * @brief Start the plugin
   *
   * Calls StartHook() which can be overridden by the derived classes.
   * @return true if we started ok, false otherwise
   */
  virtual bool Start() = 0;

  /**
   * @brief Stop the plugin
   *
   * Calls StopHook() which can be overridden by the derived classes.
   * @return true on success, false on failure
   */
  virtual bool Stop() = 0;

  /**
   * @brief Get the plugin ID of this plugin
   * @return the ola_plugin_id of this plugin
   */
  virtual ola_plugin_id Id() const = 0;

  /**
   * @brief Get the plugin name
   * @return the name of this plugin
   */
  virtual std::string Name() const = 0;

  /**
   * Return the description for this plugin.
   * @return a string description of the plugin
   */
  virtual std::string Description() const = 0;

  virtual void ConflictsWith(std::set<ola_plugin_id> *conflict_set) const = 0;

  // used to sort plugins
  virtual bool operator<(const AbstractPlugin &other) const = 0;
};


struct PluginLessThan: public std::binary_function<AbstractPlugin*,
                                                   AbstractPlugin*, bool> {
  bool operator()(AbstractPlugin *x, AbstractPlugin *y) {
    return x->Id() < y->Id();
  }
};


class Plugin: public AbstractPlugin {
 public :
  explicit Plugin(PluginAdaptor *plugin_adaptor):
    AbstractPlugin(),
    m_plugin_adaptor(plugin_adaptor),
    m_preferences(NULL),
    m_enabled(false) {
  }
  virtual ~Plugin() {}

  bool LoadPreferences();
  std::string PreferenceConfigLocation() const;
  bool IsEnabled() const;
  void SetEnabledState(bool enable);
  virtual bool Start();
  virtual bool Stop();
  // return true if this plugin is enabled by default
  virtual bool DefaultMode() const { return true; }
  virtual ola_plugin_id Id() const = 0;

  /**
   * @brief The prefix to use for storing configuration files
   * @returns A unique prefix used to identify the configuration file for this
   *   plugin.
   */
  virtual std::string PluginPrefix() const = 0;

  // by default we don't conflict with any other plugins
  virtual void ConflictsWith(std::set<ola_plugin_id>*) const {}

  bool operator<(const AbstractPlugin &other) const {
    return Id() < other.Id();
  }

 protected:
  virtual bool StartHook() { return 0; }
  virtual bool StopHook() { return 0; }

  /**
   * Set default preferences.
   */
  virtual bool SetDefaultPreferences() { return true; }

  PluginAdaptor *m_plugin_adaptor;
  class Preferences *m_preferences;  // preferences container
  static const char ENABLED_KEY[];

 private:
  bool m_enabled;  // are we running

  DISALLOW_COPY_AND_ASSIGN(Plugin);
};
}  // namespace ola

// interface functions
typedef ola::AbstractPlugin* create_t(ola::PluginAdaptor *plugin_adaptor);

#endif  // INCLUDE_OLAD_PLUGIN_H_
