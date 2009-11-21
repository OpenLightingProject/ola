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
 * PluginLoader.h
 * Interface for the PluginLoader classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef OLAD_PLUGINLOADER_H_
#define OLAD_PLUGINLOADER_H_

#include <ola/plugin_id.h>
#include <vector>

namespace ola {

using std::vector;
class AbstractPlugin;

class PluginLoader {
  public:
    PluginLoader() {}
    virtual ~PluginLoader() {}

    void SetPluginAdaptor(class PluginAdaptor *adaptor) {
      m_plugin_adaptor = adaptor;
    }
    virtual int LoadPlugins() = 0;
    virtual int UnloadPlugins() = 0;
    virtual int PluginCount() const = 0;
    virtual vector<AbstractPlugin*> Plugins() const = 0;
    virtual AbstractPlugin* GetPlugin(ola_plugin_id plugin_id) const = 0;

  protected:
    class PluginAdaptor *m_plugin_adaptor;
};
}  // ola
#endif  // OLAD_PLUGINLOADER_H_
