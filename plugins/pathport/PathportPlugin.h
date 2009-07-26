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
 * PathPortPlugin.h
 * Interface for the pathport plugin class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef PATHPORTPLUGIN_H
#define PATHPORTPLUGIN_H

#include <olad/Plugin.h>
#include <olad/PluginAdaptor.h>
#include <ola/plugin_id.h>

namespace ola {
namespace plugin {

class PathportDevice;
using ola::Plugin;

class PathportPlugin: public Plugin {
  public:
    PathportPlugin(const ola::PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_device(NULL) {}

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_PATHPORT; }
    string PluginPrefix() const { return PLUGIN_PREFIX; }

  private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    PathportDevice *m_device; // only have one device
    static const string PATHPORT_NODE_NAME;
    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
};

} //plugin
} //ola

#endif
