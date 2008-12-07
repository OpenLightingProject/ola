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
 * LlaDaemon.h
 * Interface for the LLA Daemon class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_DAEMON_H
#define LLA_DAEMON_H

#include <lla/select_server/SelectServer.h>
#include <lla/select_server/Socket.h>

namespace lla {

using lla::select_server::ListeningSocket;
using lla::select_server::SelectServer;

class LlaDaemon {

  public :
    LlaDaemon();
    ~LlaDaemon();
    bool Init();
    int Run();
    void Terminate();
    void ReloadPlugins();

  private :
    LlaDaemon(const LlaDaemon&);
    LlaDaemon& operator=(const LlaDaemon&);

    class PluginLoader *m_plugin_loader;
    class SelectServer *m_ss;
    class LlaServer *m_server;
    class PreferencesFactory *m_preferences_factory;
    class ListeningSocket *m_listening_socket;
    class LlaServerServiceImplFactory *m_service_factory;
};

} // lla
#endif
