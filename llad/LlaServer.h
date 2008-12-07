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
 * LlaServer.h
 * Interface for the lla server class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_SERVER_H
#define LLA_SERVER_H

#include <map>

#include <lla/select_server/Socket.h>
#include <lla/select_server/SelectServer.h>
#include <lla/plugin_id.h>
#include <llad/Preferences.h>

#include "common/protocol/Lla.pb.h"
#include "llad/LlaServerServiceImpl.h"

namespace lla {

/*
 * The main LlaServer class
 */
class LlaServer: public lla::select_server::AcceptSocketListener,
                 public lla::select_server::SocketManager {
  public:
    LlaServer(LlaServerServiceImplFactory *factory,
              class PluginLoader *plugin_loader,
              class PreferencesFactory *preferences_factory,
              lla::select_server::SelectServer *select_server,
              lla::select_server::ListeningSocket *socket=NULL);
    ~LlaServer();
    bool Init();
    void ReloadPlugins();
    int NewConnection(lla::select_server::ConnectedSocket *socket);
    void SocketClosed(lla::select_server::Socket *socket);

  private :
    LlaServer(const LlaServer&);
    LlaServer& operator=(const LlaServer&);
    void StopPlugins();
    void StartPlugins();
    void CleanupConnection(LlaServerServiceImpl *service);

    LlaServerServiceImplFactory *m_service_factory;
    class PluginLoader *m_plugin_loader;
    lla::select_server::SelectServer *m_ss;
    lla::select_server::ListeningSocket *m_listening_socket;

    class DeviceManager *m_device_manager;
    class PluginAdaptor *m_plugin_adaptor;
    class PreferencesFactory *m_preferences_factory;
    class Preferences *m_universe_preferences;
    class UniverseStore *m_universe_store;

    static const string UNIVERSE_PREFERENCES;
    bool m_init_run;
    std::map<int, class LlaServerServiceImpl*> m_sd_to_service;
};


} // lla
#endif
