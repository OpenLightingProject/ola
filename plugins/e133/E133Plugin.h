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
 * E133Plugin.h
 * Interface for the E1.33 plugin class
 * Copyright (C) 2024 Peter Newman
 */

#ifndef PLUGINS_E133_E133PLUGIN_H_
#define PLUGINS_E133_E133PLUGIN_H_

#include <string>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace e133 {

class E133Device;

class E133Plugin: public ola::Plugin {
 public:
    explicit E133Plugin(ola::PluginAdaptor *plugin_adaptor):
      ola::Plugin(plugin_adaptor),
      m_device(NULL) {}
    ~E133Plugin() {}

    std::string Name() const { return PLUGIN_NAME; }
    ola_plugin_id Id() const { return OLA_PLUGIN_E133; }
    std::string Description() const;
    std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    E133Device *m_device;
    static const char CID_KEY[];
    static const unsigned int DEFAULT_DSCP_VALUE;
    static const unsigned int DEFAULT_PORT_COUNT;
    static const char DSCP_KEY[];
    static const char INPUT_PORT_COUNT_KEY[];
    static const char IP_KEY[];
    static const char OUTPUT_PORT_COUNT_KEY[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char PREPEND_HOSTNAME_KEY[];
    static const char TARGET_SOCKET_KEY[];
};
}  // namespace e133
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E133_E133PLUGIN_H_
