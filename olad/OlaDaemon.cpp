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
 * OlaDaemon.cpp
 * This is the main ola daemon
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdio.h>
#include <string.h>
#include <vector>

#include "config.h"
#include "ola/ExportMap.h"

#include "olad/DynamicPluginLoader.h"
#include "olad/OlaDaemon.h"
#include "olad/OlaServerServiceImpl.h"
#include "olad/PluginLoader.h"
#include "olad/Preferences.h"

namespace ola {

using ola::network::TcpAcceptingSocket;
using ola::network::AcceptingSocket;
using ola::network::SelectServer;

const char OlaDaemon::K_RPC_PORT_VAR[] = "rpc-port";

/*
 * Create a new OlaDaemon
 * @param PluginLoader what to use to access the plugins
 */
OlaDaemon::OlaDaemon(const ola_server_options &options,
                     ExportMap *export_map,
                     unsigned int rpc_port)
    : m_ss(NULL),
      m_server(NULL),
      m_preferences_factory(NULL),
      m_accepting_socket(NULL),
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
OlaDaemon::~OlaDaemon() {
  Shutdown();
}


/*
 * Initialise this object
 * @return true on success, false on failure
 */
bool OlaDaemon::Init() {
  if (m_server || m_service_factory || m_preferences_factory ||
      m_accepting_socket || m_server)
    return false;

  m_ss = new SelectServer(m_export_map);
  m_service_factory = new OlaServerServiceImplFactory();

  // Order is important here as we won't load the same plugin twice.
  m_plugin_loaders.push_back(new DynamicPluginLoader());

  m_preferences_factory = new FileBackedPreferencesFactory();
  m_accepting_socket = new TcpAcceptingSocket("127.0.0.1", m_rpc_port);

  m_server = new OlaServer(m_service_factory,
                           m_plugin_loaders,
                           m_preferences_factory,
                           m_ss,
                           &m_options,
                           m_accepting_socket,
                           m_export_map);
  return m_server->Init();
}


/*
 * Shutdown the daemon
 */
void OlaDaemon::Shutdown() {
  delete m_server;
  delete m_service_factory;
  delete m_preferences_factory;
  delete m_ss;
  delete m_accepting_socket;
  m_accepting_socket = NULL;
  m_preferences_factory = NULL;
  m_server = NULL;
  m_service_factory = NULL;
  m_ss = NULL;

  vector<PluginLoader*>::iterator iter;
  for (iter = m_plugin_loaders.begin(); iter != m_plugin_loaders.end(); ++iter)
    delete *iter;
  m_plugin_loaders.clear();
}


/*
 * Run the daemon
 */
void OlaDaemon::Run() {
  if (m_ss)
    m_ss->Run();
}


/*
 * Stop the daemon
 */
void OlaDaemon::Terminate() {
  if (m_ss)
    m_ss->Terminate();
}

/*
 * Reload plugins
 */
void OlaDaemon::ReloadPlugins() {
  if (m_server)
    m_server->ReloadPlugins();
}
}  // ola
