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

#include <llad/logger.h>
#include "LlaDaemon.h"
#include "LlaServer.h"
#include "DynamicPluginLoader.h"

#include "PluginLoader.h"

namespace lla {

using lla::select_server::TcpListeningSocket;
using lla::select_server::ListeningSocket;
using lla::select_server::SelectServer;


/*
 * Create a new LlaDaemon
 *
 * @param PluginLoader what to use to access the plugins
 */
LlaDaemon::LlaDaemon():
  m_plugin_loader(NULL),
  m_ss(NULL),
  m_server(NULL),
  m_preferences_factory(NULL),
  m_listening_socket(NULL),
  m_service_factory(NULL) {
}

/*
 * Destroy this object
 *
 */
LlaDaemon::~LlaDaemon() {
  m_listening_socket->Close();

  delete m_preferences_factory;
  delete m_service_factory;
  delete m_plugin_loader;
  delete m_server;
  delete m_ss;
  delete m_listening_socket;
}


/*
 * Initialise this object
 *
 * @return 0 on success, -1 on failure
 */
bool LlaDaemon::Init() {
  m_ss = new SelectServer();
  m_service_factory = new LlaServerServiceImplFactory();
  m_plugin_loader = new DynamicPluginLoader();

  m_preferences_factory = new FileBackedPreferencesFactory();
  m_listening_socket = new TcpListeningSocket("127.0.0.1", 9010);

  LlaServer *server = new LlaServer(m_service_factory,
                                    m_plugin_loader,
                                    m_preferences_factory,
                                    m_ss,
                                    m_listening_socket);
  return server->Init();
}


/*
 * Run the daemon
 */
int LlaDaemon::Run() {
  return m_ss->Run();
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
