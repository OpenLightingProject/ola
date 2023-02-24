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
 * OSCPlugin.h
 * Interface for the OSC plugin.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef PLUGINS_OSC_OSCPLUGIN_H_
#define PLUGINS_OSC_OSCPLUGIN_H_

#include <string>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"
#include "plugins/osc/OSCDevice.h"
#include "plugins/osc/OSCTarget.h"

namespace ola {
namespace plugin {
namespace osc {

class OSCDevice;

class OSCPlugin: public ola::Plugin {
 public:
    explicit OSCPlugin(ola::PluginAdaptor *plugin_adaptor):
      ola::Plugin(plugin_adaptor),
      m_device(NULL) {}

    std::string Name() const { return PLUGIN_NAME; }
    std::string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_OSC; }
    std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    unsigned int GetPortCount(const std::string &key) const;
    bool ExtractOSCTarget(const std::string &str, OSCTarget *target);
    void SetDataFormat(const std::string &format_option,
                       OSCDevice::PortConfig *port_config);

    OSCDevice *m_device;
    static const uint8_t DEFAULT_PORT_COUNT = 5;
    static const uint16_t DEFAULT_UDP_PORT = 7770;

    static const char DEFAULT_ADDRESS_TEMPLATE[];
    static const char DEFAULT_TARGETS_TEMPLATE[];
    static const char INPUT_PORT_COUNT_KEY[];
    static const char OUTPUT_PORT_COUNT_KEY[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char PORT_ADDRESS_TEMPLATE[];
    static const char PORT_TARGETS_TEMPLATE[];
    static const char PORT_FORMAT_TEMPLATE[];
    static const char UDP_PORT_KEY[];

    static const char BLOB_FORMAT[];
    static const char FLOAT_ARRAY_FORMAT[];
    static const char FLOAT_INDIVIDUAL_FORMAT[];
    static const char INT_ARRAY_FORMAT[];
    static const char INT_INDIVIDUAL_FORMAT[];
};
}  // namespace osc
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OSC_OSCPLUGIN_H_
