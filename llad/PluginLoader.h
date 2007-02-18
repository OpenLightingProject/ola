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
 * Interface for the pluginloader classes
 * Copyright (C) 2005-2007  Simon Newton
 */

#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

#include <map>
#include <vector>

#include <llad/plugin.h>

using namespace std;

class PluginLoader {

  public:
    PluginLoader() {};
    virtual ~PluginLoader() {};

    void set_plugin_adaptor(class PluginAdaptor *pa) { m_pa = pa; }
    virtual int load_plugins() = 0;
    virtual int unload_plugins() { return 0; }
    virtual int plugin_count() const = 0;
    virtual Plugin *get_plugin(unsigned int id) const = 0;

  protected:
    class PluginAdaptor *m_pa;
};

#endif
