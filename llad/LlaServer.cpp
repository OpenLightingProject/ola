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
 * LlaServer.cpp
 * LlaServer is the main LLA Server class
 * Copyright (C) 2005-2008 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <lla/ExportMap.h>
#include <lla/Logging.h>
#include <llad/Plugin.h>
#include <llad/PluginAdaptor.h>
#include <llad/Preferences.h>
#include <llad/Preferences.h>
#include <llad/Universe.h>
#include "common/protocol/Lla.pb.h"
#include "common/rpc/StreamRpcChannel.h"

#include "Client.h"
#include "DeviceManager.h"
#include "LlaServer.h"
#include "LlaServerServiceImpl.h"
#include "LlaServerServiceImpl.h"
#include "PluginLoader.h"
#include "UniverseStore.h"

#ifdef HAVE_LIBMICROHTTPD
#include "LlaHttpServer.h"
#endif

namespace lla {

using lla::rpc::StreamRpcChannel;

const string LlaServer::UNIVERSE_PREFERENCES = "universe";
const string LlaServer::K_CLIENT_VAR = "clients-connected";
const unsigned int LlaServer::K_GARBAGE_COLLECTOR_TIMEOUT_MS = 5000;


/*
 * Create a new LlaServer
 *
 * @param factory the factory to use to create LlaService objects
 * @param m_plugin_loader the loader to use for the plugins
 * @param socket the socket to listen on for new connections
 */
LlaServer::LlaServer(LlaServerServiceImplFactory *factory,
                     PluginLoader *plugin_loader,
                     PreferencesFactory *preferences_factory,
                     lla::network::SelectServer *select_server,
                     lla_server_options *lla_options,
                     lla::network::AcceptingSocket *socket,
                     ExportMap *export_map):
  m_service_factory(factory),
  m_plugin_loader(plugin_loader),
  m_ss(select_server),
  m_accepting_socket(socket),
  m_device_manager(NULL),
  m_plugin_adaptor(NULL),
  m_preferences_factory(preferences_factory),
  m_universe_preferences(NULL),
  m_universe_store(NULL),
  m_export_map(export_map),
  m_init_run(false),
  m_free_export_map(false),
  m_httpd(NULL),
  m_options(*lla_options) {

  if (!m_export_map) {
    m_export_map = new ExportMap();
    m_free_export_map = true;
  }

  if (!m_options.http_port)
    m_options.http_port = DEFAULT_HTTP_PORT;

  m_export_map->GetIntegerVar(K_CLIENT_VAR);
}


/*
 * Shutdown the server
 */
LlaServer::~LlaServer() {

#ifdef HAVE_LIBMICROHTTPD
  if (m_httpd) {
    m_httpd->Stop();
    delete m_httpd;
    m_httpd = NULL;
  }
#endif

  // stops and unloads all our plugins
  if (m_plugin_loader) {
    m_plugin_loader->SetPluginAdaptor(NULL);
    StopPlugins();
  }

  map<int, LlaServerServiceImpl*>::iterator iter;
  for (iter = m_sd_to_service.begin(); iter != m_sd_to_service.end(); ++iter) {

    CleanupConnection(iter->second);
    //TODO: close the socket here

    /*Socket *socket = ;
    m_ss->RemoveSocket(socket);
    socket->Close();
    */
  }

  if (m_accepting_socket)
    m_ss->RemoveSocket(m_accepting_socket);

  if (m_universe_store) {
    m_universe_store->DeleteAll();
    delete m_universe_store;
  }

  if (m_universe_preferences) {
    m_universe_preferences->Save();
  }

  delete m_plugin_adaptor;
  delete m_device_manager;

  if (m_free_export_map)
    delete m_export_map;
}


/*
 * Initialise the server
 * * @return true on success, false on failure
 */
bool LlaServer::Init() {
  if (m_init_run)
    return false;

  if (!m_service_factory || !m_ss)
    return false;

  //TODO: run without preferences & PluginLoader
  if (!m_plugin_loader || !m_preferences_factory)
    return false;

  if (m_accepting_socket) {
    if (!m_accepting_socket->Listen())
      return false;
    m_ss->AddSocket(m_accepting_socket,
                    lla::NewClosure(this, &LlaServer::AcceptNewConnection,
                                    m_accepting_socket));
  }

  signal(SIGPIPE, SIG_IGN);

  m_universe_preferences = m_preferences_factory->NewPreference(
      UNIVERSE_PREFERENCES);
  m_universe_preferences->Load();
  m_universe_store = new UniverseStore(m_universe_preferences, m_export_map);

  // setup the objects
  m_device_manager = new DeviceManager();
  m_plugin_adaptor = new PluginAdaptor(m_device_manager,
                                       m_ss,
                                       m_preferences_factory);

  if (!m_universe_store || !m_device_manager || !m_plugin_adaptor) {
    delete m_universe_store;
    delete m_device_manager;
    delete m_plugin_adaptor;
    return false;
  }

  m_plugin_loader->SetPluginAdaptor(m_plugin_adaptor);
  StartPlugins();

#ifdef HAVE_LIBMICROHTTPD
  if (m_options.http_enable) {
    m_httpd = new LlaHttpServer(m_export_map,
                                m_ss,
                                m_universe_store,
                                m_plugin_loader,
                                m_device_manager,
                                m_options.http_port,
                                m_options.http_enable_quit,
                                m_options.http_data_dir);
    m_httpd->Start();
  }
#endif

  m_ss->RegisterRepeatingTimeout(
      K_GARBAGE_COLLECTOR_TIMEOUT_MS,
      lla::NewClosure(this, &LlaServer::GarbageCollect));

  m_init_run = true;
  return true;
}


/*
 * Reload all plugins
 */
void LlaServer::ReloadPlugins() {
  LLA_INFO << "Reloading plugins";

  if (!m_plugin_loader)
    return;

  StopPlugins();
  StartPlugins();
}


/*
 * Add a new ConnectedSocket to this Server.
 * @param socket the new ConnectedSocket
 */
int LlaServer::AcceptNewConnection(
    lla::network::AcceptingSocket *accepting_socket) {
  lla::network::ConnectedSocket *socket = accepting_socket->Accept();

  if (!socket)
    return 0;

  StreamRpcChannel *channel = new StreamRpcChannel(NULL, socket);
  socket->SetOnClose(NewClosure(this, &LlaServer::SocketClosed, socket));
  LlaClientService_Stub *stub = new LlaClientService_Stub(channel);
  Client *client = new Client(stub);
  LlaServerServiceImpl *service = m_service_factory->New(m_universe_store,
                                                         m_device_manager,
                                                         m_plugin_loader,
                                                         client,
                                                         m_export_map);
  channel->SetService(service);

  map<int, LlaServerServiceImpl*>::const_iterator iter;
  iter = m_sd_to_service.find(socket->ReadDescriptor());

  if (iter != m_sd_to_service.end())
    LLA_INFO << "New socket but the client already exists!";

  pair<int, LlaServerServiceImpl*> pair(socket->ReadDescriptor(), service);
  m_sd_to_service.insert(pair);

  m_ss->AddSocket(socket, true);
  IntegerVariable *var = m_export_map->GetIntegerVar(K_CLIENT_VAR);
  var->Increment();
  return 0;
}


/*
 * Called when a socket is closed
 */
int LlaServer::SocketClosed(lla::network::ConnectedSocket *socket) {
  map<int, LlaServerServiceImpl*>::iterator iter;
  iter = m_sd_to_service.find(socket->ReadDescriptor());

  if (iter == m_sd_to_service.end())
    LLA_INFO << "A socket was closed but we didn't find the client";

  IntegerVariable *var = m_export_map->GetIntegerVar(K_CLIENT_VAR);
  var->Decrement();
  CleanupConnection(iter->second);
  m_sd_to_service.erase(iter);
  return 0;
}


/*
 * Run the garbage collector
 */
int LlaServer::GarbageCollect() {
  LLA_INFO << "Garbage collecting";
  m_universe_store->GarbageCollectUniverses();
  return 0;
}


/*
 * Load and start the plugins
 */
void LlaServer::StartPlugins() {
  m_plugin_loader->LoadPlugins();

  vector<AbstractPlugin*> plugins = m_plugin_loader->Plugins();
  vector<AbstractPlugin*>::iterator iter;

  for (iter = plugins.begin(); iter != plugins.end(); ++iter) {
    LLA_INFO << "Trying to start " << (*iter)->Name();
    if (!(*iter)->Start())
      LLA_WARN << "Failed to start " << (*iter)->Name();
    else
      LLA_INFO << "Started " << (*iter)->Name();
  }
}


/*
 * Stop and unload all the plugins
 */
void LlaServer::StopPlugins() {
  m_plugin_loader->UnloadPlugins();
  if (m_device_manager) {
    if (m_device_manager->DeviceCount()) {
      LLA_WARN << "Some devices failed to unload, we're probably leaking "
        << "memory now";
    }
    m_device_manager->UnregisterAllDevices();
  }
}


/*
 * Cleanup everything related to a client connection
 */
void LlaServer::CleanupConnection(LlaServerServiceImpl *service) {
  Client *client = service->GetClient();

  vector<Universe*> *universe_list = m_universe_store->GetList();
  vector<Universe*>::iterator uni_iter;

  // O(universes * clients). Clean this up sometime.
  for (uni_iter = universe_list->begin();
       uni_iter != universe_list->end();
       ++uni_iter) {
    (*uni_iter)->RemoveSourceClient(client);
    (*uni_iter)->RemoveSinkClient(client);
  }
  delete universe_list;
  delete client->Stub()->channel();
  delete client->Stub();
  delete client;
  delete service;
}

} // lla
