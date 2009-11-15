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
 * Plugin.h
 * Header file for plugin class - plugins inherit from this.
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef INCLUDE_OLAD_PLUGIN_H_
#define INCLUDE_OLAD_PLUGIN_H_

#include <string>
#include <functional>
#include <ola/plugin_id.h>  // NOLINT

namespace ola {

using std::string;
class PluginAdaptor;

/*
 * The interface for a plugin
 */
class AbstractPlugin {
  public :
    AbstractPlugin() {}
    virtual ~AbstractPlugin() {}

    // start the plugin
    virtual bool Start() = 0;
    // stop the plugin
    virtual bool Stop() = 0;
    // check if this plugin is enabled
    virtual bool IsEnabled() const = 0;
    // return the plugin_id of this plugin
    virtual ola_plugin_id Id() const = 0;
    // return the name of this plugin
    virtual string Name() const = 0;
    // return the description of this plugin
    virtual string Description() const = 0;

    // used to sort plugins
    virtual bool operator<(const AbstractPlugin &other) const = 0;
};


struct PluginLessThan: public std::binary_function<AbstractPlugin*,
                                                   AbstractPlugin*, bool> {
  bool operator()(AbstractPlugin *x, AbstractPlugin *y) {
    return *x < *y;
  }
};


class Plugin: public AbstractPlugin {
  public :
    explicit Plugin(const PluginAdaptor *plugin_adaptor):
      AbstractPlugin(),
      m_plugin_adaptor(plugin_adaptor),
      m_preferences(NULL),
      m_enabled(false),
      m_debug(false) {}

    virtual ~Plugin() {}

    virtual bool Start();
    virtual bool Stop();
    virtual bool IsEnabled() const { return m_enabled; }
    virtual ola_plugin_id Id() const = 0;

    // return the prefix used to identify this plugin
    virtual string PluginPrefix() const = 0;

    bool operator<(const AbstractPlugin &other) const {
      return Id() < other.Id();
    }

  protected:
    virtual bool StartHook() { return 0; }
    virtual bool StopHook() { return 0; }
    virtual bool SetDefaultPreferences() { return true; }

    const PluginAdaptor *m_plugin_adaptor;
    class Preferences *m_preferences;  // preferences container
    bool m_enabled;  // are we running
    bool m_debug;  // debug mode on
    static const string ENABLED_KEY;
    static const string DEBUG_KEY;

  private:
    bool LoadPreferences();
    Plugin(const Plugin&);
    Plugin& operator=(const Plugin&);
};


}  // ola

// interface functions
typedef ola::AbstractPlugin* create_t(const ola::PluginAdaptor *plugin_adaptor);
typedef void destroy_t(ola::AbstractPlugin*);

#endif  // INCLUDE_OLAD_PLUGIN_H_
