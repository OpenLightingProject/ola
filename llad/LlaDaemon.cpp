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
 * LlaDaemon.cpp
 * This is the main lla daemon
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdio.h>
#include <string.h>

#include <lla/ExportMap.h>
#include <llad/LlaDaemon.h>
#include <llad/Preferences.h>
#include <llad/logger.h>
#include "DlOpenPluginLoader.h"
#include "LlaServerServiceImpl.h"
#include "PluginLoader.h"

namespace lla {

using lla::select_server::TcpListeningSocket;
using lla::select_server::ListeningSocket;
using lla::select_server::SelectServer;

const string LlaDaemon::K_RPC_PORT_VAR = "rpc_port";

/*
 * Create a new LlaDaemon
 *
 * @param PluginLoader what to use to access the plugins
 */
LlaDaemon::LlaDaemon(lla_server_options &options,
                     ExportMap *export_map,
                     unsigned int rpc_port):
  m_plugin_loader(NULL),
  m_ss(NULL),
  m_server(NULL),
  m_preferences_factory(NULL),
  m_listening_socket(NULL),
  m_service_factory(NULL),
  m_options(options),
  m_export_map(export_map),
  m_rpc_port(rpc_port) {

  if (m_export_map) {
    IntegerVariable *var = m_export_map->GetIntegerVar(K_RPC_PORT_VAR);
    var->Set(rpc_port);
  }
}


/*
 * Destroy this object
 *
 */
LlaDaemon::~LlaDaemon() {
  m_listening_socket->Close();
  delete m_preferences_factory;
  delete m_service_factory;
  delete m_server;
  delete m_plugin_loader;
  delete m_ss;
  delete m_listening_socket;
}


/*
 * Initialise this object
 *
 * @return 0 on success, -1 on failure
 */
bool LlaDaemon::Init() {
  m_ss = new SelectServer(m_export_map);
  m_service_factory = new LlaServerServiceImplFactory();
  m_plugin_loader = new DlOpenPluginLoader(PLUGIN_DIR);

  m_preferences_factory = new FileBackedPreferencesFactory();
  m_listening_socket = new TcpListeningSocket("127.0.0.1", m_rpc_port);

  m_server = new LlaServer(m_service_factory,
                           m_plugin_loader,
                           m_preferences_factory,
                           m_ss,
                           &m_options,
                           m_listening_socket,
                           m_export_map);
  return m_server->Init();
}


/*
 * Run the daemon
 */
void LlaDaemon::Run() {
  m_ss->Run();
}


/*
 * Stop the daemon
 */
void LlaDaemon::Terminate() {
  m_ss->Terminate();
}

/*
 * Reload plugins
 */
void LlaDaemon::ReloadPlugins() {
  m_server->ReloadPlugins();
}

} //lla
