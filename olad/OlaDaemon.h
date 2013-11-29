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
 * OlaDaemon.h
 * Interface for the OLA Daemon class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef OLAD_OLADAEMON_H_
#define OLAD_OLADAEMON_H_

#include <memory>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/ExportMap.h"
#include "ola/base/Macro.h"
#include "ola/io/SelectServer.h"
#include "ola/network/Socket.h"
#include "ola/network/SocketAddress.h"
#include "olad/OlaServer.h"

namespace ola {
namespace rdm {
  class RootPidStore;
}  // rdm

using ola::io::SelectServer;
using ola::network::TCPAcceptingSocket;
using std::auto_ptr;

class OlaDaemon {
 public:
    OlaDaemon(const OlaServer::Options &options,
              ExportMap *export_map = NULL);
    ~OlaDaemon();
    bool Init();
    void Shutdown();
    void Run();

    ola::network::GenericSocketAddress RPCAddress() const;
    SelectServer* GetSelectServer() { return &m_ss; }
    OlaServer *GetOlaServer() const { return m_server.get(); }

    static const unsigned int DEFAULT_RPC_PORT = OLA_DEFAULT_PORT;

 private:
    const OlaServer::Options m_options;
    class ExportMap *m_export_map;
    SelectServer m_ss;
    vector<class PluginLoader*> m_plugin_loaders;

    auto_ptr<class PreferencesFactory> m_preferences_factory;
    auto_ptr<class OlaClientServiceFactory> m_service_factory;
    auto_ptr<TCPAcceptingSocket> m_accepting_socket;
    auto_ptr<OlaServer> m_server;

    string DefaultConfigDir();
    bool InitConfigDir(const string &path);

    static const char K_RPC_PORT_VAR[];
    static const char OLA_CONFIG_DIR[];

    DISALLOW_COPY_AND_ASSIGN(OlaDaemon);
};
}  // namespace ola
#endif  // OLAD_OLADAEMON_H_
