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
 * DlOpenPluginLoader.h
 * Interface for the DlOpenPluginLoader class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef DLOPENPLUGINLOADER_H
#define DLOPENPLUGINLOADER_H

#include <map>
#include <string>
#include <vector>

#include "PluginLoader.h"

namespace lla {

using std::vector;
using std::map;
using std::string;

class AbstractPlugin;

class DlOpenPluginLoader: public PluginLoader {

  public:
    DlOpenPluginLoader(const string &dirname): m_dirname(dirname) {};
    ~DlOpenPluginLoader() { UnloadPlugins(); }

    int LoadPlugins();
    int UnloadPlugins();
    int PluginCount() const;
    AbstractPlugin *GetPlugin(unsigned int plugin_id) const;
    vector<AbstractPlugin*> Plugins() const { return m_plugins; }

  private:
    DlOpenPluginLoader(const DlOpenPluginLoader&);
    DlOpenPluginLoader operator=(const DlOpenPluginLoader&);

    AbstractPlugin *LoadPlugin(const string &path);
    int UnloadPlugin(void *handle);

    string m_dirname;
    std::vector<AbstractPlugin*> m_plugins;
    std::map<void*, AbstractPlugin*> m_plugin_map;
};

} //lla
#endif
