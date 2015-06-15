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
 * PathportPlugin.h
 * Interface for the pathport plugin class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_PATHPORT_PATHPORTPLUGIN_H_
#define PLUGINS_PATHPORT_PATHPORTPLUGIN_H_

#include <stdint.h>
#include <string>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace pathport {

class PathportPlugin: public ola::Plugin {
 public:
    explicit PathportPlugin(class ola::PluginAdaptor *plugin_adaptor)
        : Plugin(plugin_adaptor),
          m_device(NULL) {}

    std::string Name() const { return PLUGIN_NAME; }
    std::string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_PATHPORT; }
    std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    class PathportDevice *m_device;

    static const unsigned int DEFAULT_DSCP_VALUE;
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    // 0x28 is assigned to the OLA project
    static const uint8_t OLA_MANUFACTURER_CODE = 0x28;
};
}  // namespace pathport
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_PATHPORT_PATHPORTPLUGIN_H_
