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
 * StageProfiPlugin.h
 * Interface for the stageprofi plugin class
 * Copyright (C) 2006-2008 Simon Newton
 */

#ifndef PLUGINS_STAGEPROFI_STAGEPROFIPLUGIN_H_
#define PLUGINS_STAGEPROFI_STAGEPROFIPLUGIN_H_

#include <vector>
#include <string>
#include "olad/Plugin.h"
#include "ola/network/Socket.h"
#include "ola/plugin_id.h"

namespace ola {
namespace plugin {
namespace stageprofi {

using ola::io::ConnectedDescriptor;
using std::string;

class StageProfiDevice;

class StageProfiPlugin: public Plugin {
  public:
    explicit StageProfiPlugin(PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor) {}
    ~StageProfiPlugin() {}

    string Name() const { return PLUGIN_NAME; }
    ola_plugin_id Id() const { return OLA_PLUGIN_STAGEPROFI; }
    string Description() const;
    int SocketClosed(ConnectedDescriptor *socket);
    string PluginPrefix() const { return PLUGIN_PREFIX; }

  private:
    bool StartHook();
    bool StopHook();
    bool SetDefaultPreferences();
    void DeleteDevice(StageProfiDevice *device);

    std::vector<StageProfiDevice*> m_devices;  // list of our devices

    static const char STAGEPROFI_DEVICE_PATH[];
    static const char STAGEPROFI_DEVICE_NAME[];
    static const char PLUGIN_NAME[];
    static const char PLUGIN_PREFIX[];
    static const char DEVICE_KEY[];
};
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_STAGEPROFI_STAGEPROFIPLUGIN_H_
