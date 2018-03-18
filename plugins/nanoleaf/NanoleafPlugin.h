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
 * NanoleafPlugin.h
 * The Nanoleaf plugin for ola
 * Copyright (C) 2017 Peter Newman
 */

#ifndef PLUGINS_NANOLEAF_NANOLEAFPLUGIN_H_
#define PLUGINS_NANOLEAF_NANOLEAFPLUGIN_H_

#include <memory>
#include <string>
#include <vector>
#include "olad/Plugin.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace nanoleaf {

class NanoleafPlugin : public Plugin {
 public:
    explicit NanoleafPlugin(PluginAdaptor *plugin_adaptor);
    ~NanoleafPlugin();

    std::string Name() const { return PLUGIN_NAME; }
    ola_plugin_id Id() const { return OLA_PLUGIN_NANOLEAF; }
    std::string Description() const;
    std::string PluginPrefix() const { return PLUGIN_PREFIX; }

 private:
    std::vector<class NanoleafDevice*> m_devices;

    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();

    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char CONTROLLER_KEY[];
};
}  // namespace nanoleaf
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_NANOLEAF_NANOLEAFPLUGIN_H_
