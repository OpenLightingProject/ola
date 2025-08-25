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
 * OVDmxPlugin.h
 * The OVDMX plugin for ola
 * Copyright (C) 2005 Simon Newton, 2017 Jan Ove Saltvedt
 */

#ifndef PLUGINS_OVDMX_OVDMXPLUGIN_H_
#define PLUGINS_OVDMX_OVDMXPLUGIN_H_

#include <string>
#include <vector>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace ovdmx {

class OVDmxDevice;

class OVDmxPlugin: public Plugin {
 public:
    explicit OVDmxPlugin(PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor) {
    }

    std::string Name() const { return PLUGIN_NAME; }
    std::string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_OVDMX; }
    std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    typedef std::vector<OVDmxDevice*> DeviceList;
    DeviceList m_devices;
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char OVDMX_DEVICE_PATH[];
    static const char OVDMX_DEVICE_NAME[];
    static const char DEVICE_KEY[];
};
}  // namespace ovdmx
}  // namespace plugin
}  // namespace ola

#endif  // PLUGINS_OVDMX_OVDMXPLUGIN_H_
