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
 * OpenDmxPlugin.h
 * Interface for the opendmx plugin class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_OPENDMX_OPENDMXPLUGIN_H_
#define PLUGINS_OPENDMX_OPENDMXPLUGIN_H_

#include <string>
#include <vector>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace opendmx {

class OpenDmxDevice;

class OpenDmxPlugin: public Plugin {
 public:
    explicit OpenDmxPlugin(PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor) {
    }

    std::string Name() const { return PLUGIN_NAME; }
    std::string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_OPENDMX; }
    std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    typedef std::vector<OpenDmxDevice*> DeviceList;
    DeviceList m_devices;
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char OPENDMX_DEVICE_PATH[];
    static const char OPENDMX_DEVICE_NAME[];
    static const char DEVICE_KEY[];
};
}  // namespace opendmx
}  // namespace plugin
}  // namespace ola

#endif  // PLUGINS_OPENDMX_OPENDMXPLUGIN_H_
