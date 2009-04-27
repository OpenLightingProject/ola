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

#ifndef LLA_PLUGIN_H
#define LLA_PLUGIN_H

#include <string>
#include <functional>
#include <lla/plugin_id.h>

namespace lla {

using std::string;
class PluginAdaptor;

class AbstractPlugin {
  public :
    AbstractPlugin() {}
    virtual ~AbstractPlugin() {};

    virtual bool Start() = 0;
    virtual bool Stop() = 0;
    virtual bool IsEnabled() const = 0;
    virtual bool DebugOn() const = 0;
    virtual lla_plugin_id Id() const = 0;
    virtual string Name() const = 0;
    virtual string Description() const = 0;

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
    Plugin(const PluginAdaptor *plugin_adaptor):
      AbstractPlugin(),
      m_plugin_adaptor(plugin_adaptor),
      m_preferences(NULL),
      m_enabled(false),
      m_debug(false) {}

    virtual ~Plugin() {};

    virtual bool Start();
    virtual bool Stop();
    virtual bool IsEnabled() const { return m_enabled; }
    virtual bool DebugOn() const { return m_debug; }
    virtual lla_plugin_id Id() const = 0;

    virtual string Name() const = 0;
    virtual string Description() const = 0;

    bool operator<(const AbstractPlugin &other) const {
      return Id() < other.Id();
    }

  protected:
    virtual bool StartHook() { return 0; }
    virtual bool StopHook() { return 0; }
    virtual int SetDefaultPreferences() { return 0; }
    virtual string PreferencesSuffix() const = 0;

    const PluginAdaptor *m_plugin_adaptor;
    class Preferences *m_preferences;  // preferences container
    bool m_enabled; // are we running
    bool m_debug; // debug mode on
    static const string ENABLED_KEY;
    static const string DEBUG_KEY;

  private:
    bool LoadPreferences();
    Plugin(const Plugin&);
    Plugin& operator=(const Plugin&);
};


} // lla

// interface functions
typedef lla::AbstractPlugin* create_t(const lla::PluginAdaptor *plugin_adaptor);
typedef void destroy_t(lla::AbstractPlugin*);

#endif
