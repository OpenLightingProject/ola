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
 * SandNetPlugin.h
 * Interface for the sandnet plugin class
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef PLUGINS_SANDNET_SANDNETPLUGIN_H_
#define PLUGINS_SANDNET_SANDNETPLUGIN_H_

#include <string>
#include "ola/plugin_id.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"

namespace ola {
namespace plugin {
namespace sandnet {

class SandNetPlugin: public ola::Plugin {
  public:
    explicit SandNetPlugin(const ola::PluginAdaptor *plugin_adaptor)
        : Plugin(plugin_adaptor),
          m_device(NULL) {}

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_SANDNET; }
    string PluginPrefix() const { return PLUGIN_PREFIX; }

  private:
    class SandNetDevice *m_device;  // only have one device

    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    static const char SANDNET_NODE_NAME[];
    static const char SANDNET_DEVICE_NAME[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
};
}  // sandnet
}  // plugin
}  // ola
#endif  // PLUGINS_SANDNET_SANDNETPLUGIN_H_
