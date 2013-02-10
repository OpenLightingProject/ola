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
 * OSCPlugin.h
 * Interface for the OSC plugin.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef PLUGINS_OSC_OSCPLUGIN_H_
#define PLUGINS_OSC_OSCPLUGIN_H_

#include <string>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"
#include "plugins/osc/OSCTarget.h"

namespace ola {
namespace plugin {
namespace osc {

using ola::Plugin;
using ola::PluginAdaptor;

class OSCDevice;

class OSCPlugin: public Plugin {
  public:
    explicit OSCPlugin(PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor),
      m_device(NULL) {}

    string Name() const { return PLUGIN_NAME; }
    string Description() const;
    ola_plugin_id Id() const { return OLA_PLUGIN_OSC; }
    string PluginPrefix() const { return PLUGIN_PREFIX; }

  private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    unsigned int GetPortCount(const string &key) const;
    bool ExtractOSCTarget(const string &str, OSCTarget *target);

    OSCDevice *m_device;
    static const char DEFAULT_ADDRESS_TEMPLATE[];
    static const char DEFAULT_PORT_COUNT[];
    static const char DEFAULT_TARGETS_TEMPLATE[];
    static const char DEFAULT_UDP_PORT[];
    static const char INPUT_PORT_COUNT_KEY[];
    static const char OUTPUT_PORT_COUNT_KEY[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char PORT_ADDRESS_TEMPLATE[];
    static const char PORT_TARGETS_TEMPLATE[];
    static const char UDP_PORT_KEY[];
};
}  // osc
}  // plugin
}  // ola
#endif  // PLUGINS_OSC_OSCPLUGIN_H_
