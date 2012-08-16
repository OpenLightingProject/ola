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
 * PluginManager.h
 * Interface to the PluginManager class
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef OLAD_PLUGINMANAGER_H_
#define OLAD_PLUGINMANAGER_H_

#include <map>
#include <vector>

namespace ola {

using std::vector;

class PluginLoader;
class PluginAdaptor;
class AbstractPlugin;

class PluginManager {
  public:
    PluginManager(const vector<PluginLoader*> &plugin_loaders,
                  PluginAdaptor *plugin_adaptor);
    ~PluginManager();

    void LoadAll();
    void UnloadAll();
    // Return a list of all loaded plugins
    void Plugins(vector<AbstractPlugin*> *plugins) const;

    // Return a list of all enabled plugins
    void EnabledPlugins(vector<AbstractPlugin*> *plugins) const;

    // Lookup a loaded plugin by ID
    AbstractPlugin* GetPlugin(ola_plugin_id plugin_id) const;

  private:
    PluginManager(const PluginManager&);
    PluginManager operator=(const PluginManager&);

    typedef std::map<ola_plugin_id, AbstractPlugin*> PluginMap;

    vector<PluginLoader*> m_plugin_loaders;
    PluginMap m_loaded_plugins;  // plugins that are loaded
    vector<AbstractPlugin*> m_enabled_plugins;  // enabled plugins
    PluginAdaptor *m_plugin_adaptor;
};
}  // ola
#endif  // OLAD_PLUGINMANAGER_H_
