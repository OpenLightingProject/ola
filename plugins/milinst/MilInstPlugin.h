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
 * MilInstPlugin.h
 * Interface for the Milford Instruments plugin class
 * Copyright (C) 2013 Peter Newman
 */

#ifndef PLUGINS_MILINST_MILINSTPLUGIN_H_
#define PLUGINS_MILINST_MILINSTPLUGIN_H_

#include <string>
#include <vector>

#include "ola/io/Descriptor.h"
#include "olad/Plugin.h"

namespace ola {
namespace plugin {
namespace milinst {

class MilInstDevice;

class MilInstPlugin: public Plugin {
 public:
    explicit MilInstPlugin(PluginAdaptor *plugin_adaptor)
        : Plugin(plugin_adaptor) {}

    ~MilInstPlugin() {}

    std::string Name() const { return PLUGIN_NAME; }
    ola_plugin_id Id() const { return OLA_PLUGIN_MILINST; }
    std::string Description() const;
    int SocketClosed(ola::io::ConnectedDescriptor *socket);
    std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();
    void DeleteDevice(MilInstDevice *device);

    std::vector<MilInstDevice*> m_devices;  // list of our devices

    static const char MILINST_DEVICE_PATH[];
    static const char MILINST_BASE_DEVICE_NAME[];
    static const char MILINST_1463_DEVICE_NAME[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char DEVICE_KEY[];
};
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_MILINST_MILINSTPLUGIN_H_
