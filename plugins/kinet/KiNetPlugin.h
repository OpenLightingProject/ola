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
 * KiNetPlugin.cpp
 * The KiNET plugin for ola
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_KINET_KINETPLUGIN_H_
#define PLUGINS_KINET_KINETPLUGIN_H_

#include <memory>
#include <string>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace kinet {

using ola::Plugin;
using ola::PluginAdaptor;
using std::string;

class KiNetPlugin : public Plugin {
  public:
    explicit KiNetPlugin(PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor) {}

    ~KiNetPlugin() {}

    string Name() const { return PLUGIN_NAME; }
    ola_plugin_id Id() const { return OLA_PLUGIN_KINET; }
    string Description() const;
    string PluginPrefix() const { return PLUGIN_PREFIX; }

  private:
    std::auto_ptr<class KiNetDevice> m_device;  // only have one device

    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char POWER_SUPPLY_KEY[];
};
}  // namespace kinet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_KINET_KINETPLUGIN_H_
