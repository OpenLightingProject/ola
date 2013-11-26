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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RenardPlugin.h
 * Interface for the renard plugin class
 * Copyright (C) 2013 Hakan Lindestaf
 */

#ifndef PLUGINS_RENARD_RENARDPLUGIN_H_
#define PLUGINS_RENARD_RENARDPLUGIN_H_

#include <string>
#include <vector>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace renard {

using ola::PluginAdaptor;

class RenardDevice;

class RenardPlugin: public Plugin {
  public:
    explicit RenardPlugin(PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor) {
    }

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_RENARD; }
    string PluginPrefix() const { return PLUGIN_PREFIX; }

  private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    typedef std::vector<RenardDevice*> DeviceList;
    DeviceList m_devices;
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char RENARD_DEVICE_PATH[];
    static const char RENARD_DEVICE_NAME[];
    static const char DEVICE_KEY[];
};
}  // namespace renard
}  // namespace plugin
}  // namespace ola

#endif  // PLUGINS_RENARD_RENARDPLUGIN_H_
