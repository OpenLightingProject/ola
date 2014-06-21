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
 * DummyPlugin.h
 * Interface for the dummyplugin class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_DUMMY_DUMMYPLUGIN_H_
#define PLUGINS_DUMMY_DUMMYPLUGIN_H_

#include <stdint.h>
#include <string>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace dummy {

class DummyDevice;

class DummyPlugin: public Plugin {
 public:
    explicit DummyPlugin(PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_device(NULL) {}

    std::string Name() const { return PLUGIN_NAME; }
    std::string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_DUMMY; }
    std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    DummyDevice *m_device;  // the dummy device
    static const char ACK_TIMER_COUNT_KEY[];
    static const char ADVANCED_DIMMER_KEY[];
    static const uint8_t DEFAULT_DEVICE_COUNT;
    static const uint8_t DEFAULT_ACK_TIMER_DEVICE_COUNT;
    static const uint16_t DEFAULT_SUBDEVICE_COUNT;
    static const char DEVICE_NAME[];
    static const char DIMMER_COUNT_KEY[];
    static const char DIMMER_SUBDEVICE_COUNT_KEY[];
    static const char DUMMY_DEVICE_COUNT_KEY[];
    static const char MOVING_LIGHT_COUNT_KEY[];
    static const char NETWORK_COUNT_KEY[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char SENSOR_COUNT_KEY[];
    static const char SUBDEVICE_COUNT_KEY[];
};
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_DUMMY_DUMMYPLUGIN_H_
