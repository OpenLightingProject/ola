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
 * PluginLoader.h
 * Interface for the PluginLoader classes
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_PLUGINLOADER_H_
#define OLAD_PLUGINLOADER_H_

#include <vector>

#include "ola/base/Macro.h"

namespace ola {

class PluginLoader {
 public:
    PluginLoader() {}
    virtual ~PluginLoader() {}

    void SetPluginAdaptor(class PluginAdaptor *adaptor) {
      m_plugin_adaptor = adaptor;
    }
    virtual std::vector<class AbstractPlugin*> LoadPlugins() = 0;
    virtual void UnloadPlugins() = 0;

 protected:
    class PluginAdaptor *m_plugin_adaptor;

 private:
    DISALLOW_COPY_AND_ASSIGN(PluginLoader);
};
}  // namespace ola
#endif  // OLAD_PLUGINLOADER_H_
