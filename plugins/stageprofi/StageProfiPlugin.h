/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * StageProfiPlugin.h
 * Interface for the stageprofi plugin class
 * Copyright (C) 2006-2008 Simon Newton
 */

#ifndef STAGEPROFIPLUGIN_H
#define STAGEPROFIPLUGIN_H

#include <vector>
#include <string>

#include <llad/Plugin.h>
#include <lla/select_server/Socket.h>
#include <lla/plugin_id.h>

namespace lla {
namespace plugin {

using lla::select_server::SocketManager;
using lla::select_server::Socket;
using std::string;

class StageProfiDevice;

class StageProfiPlugin: public Plugin, public SocketManager {
  public:
    StageProfiPlugin(const PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor) {}
    ~StageProfiPlugin() {}

    string Name() const { return PLUGIN_NAME; }
    lla_plugin_id Id() const { return LLA_PLUGIN_STAGEPROFI; }
    string Description() const;
    void SocketClosed(Socket *socket);

  protected:
    string PreferencesSuffix() const { return PLUGIN_PREFIX; }

  private:
    bool StartHook();
    bool StopHook();
    int SetDefaultPreferences();
    void DeleteDevice(StageProfiDevice *device);

    vector<StageProfiDevice*> m_devices;  // list of our devices

    static const string STAGEPROFI_DEVICE_PATH;
    static const string STAGEPROFI_DEVICE_NAME;
    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
};

} //plugin
} //lla
#endif
