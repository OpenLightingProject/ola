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
 * E131Plugin.h
 * Interface for the E1.31 plugin class
 * Copyright (C) 2007 Simon Newton
 */

#ifndef PLUGINS_E131_E131PLUGIN_H_
#define PLUGINS_E131_E131PLUGIN_H_

#include <string>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace e131 {

class E131Device;

class E131Plugin: public ola::Plugin {
 public:
    explicit E131Plugin(ola::PluginAdaptor *plugin_adaptor):
      ola::Plugin(plugin_adaptor),
      m_device(NULL) {}
    ~E131Plugin() {}

    std::string Name() const { return PLUGIN_NAME; }
    ola_plugin_id Id() const { return OLA_PLUGIN_E131; }
    std::string Description() const;
    std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    E131Device *m_device;
    static const char CID_KEY[];
    static const unsigned int DEFAULT_DSCP_VALUE;
    static const unsigned int DEFAULT_PORT_COUNT;
    static const char DRAFT_DISCOVERY_KEY[];
    static const char DSCP_KEY[];
    static const char IGNORE_PREVIEW_DATA_KEY[];
    static const char INPUT_PORT_COUNT_KEY[];
    static const char IP_KEY[];
    static const char INTERFACE_KEY[];
    static const char OUTPUT_PORT_COUNT_KEY[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char PREPEND_HOSTNAME_KEY[];
    static const char REVISION_0_2[];
    static const char REVISION_0_46[];
    static const char REVISION_KEY[];
};
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131PLUGIN_H_
