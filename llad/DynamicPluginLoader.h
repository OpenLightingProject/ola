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
 * DynamicPluginLoader.h
 * Interface for the DynamicPluginLoader class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_DYNAMICPLUGINLOADER_H
#define LLA_DYNAMICPLUGINLOADER_H

#include <vector>

#include "PluginLoader.h"

namespace lla {

using std::vector;
class AbstractPlugin;

class DynamicPluginLoader: public PluginLoader {
  public:
    DynamicPluginLoader() {};
    ~DynamicPluginLoader() { UnloadPlugins(); }

    int LoadPlugins();
    int UnloadPlugins();
    int PluginCount() const;
    AbstractPlugin *GetPlugin(unsigned int plugin_id) const;
    vector<AbstractPlugin*> Plugins() const;

  private:
    DynamicPluginLoader(const DynamicPluginLoader&);
    DynamicPluginLoader operator=(const DynamicPluginLoader&);

    vector<AbstractPlugin*> m_plugins;
};

} //lla
#endif
