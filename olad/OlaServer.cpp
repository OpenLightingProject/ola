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
 * OlaServer.cpp
 * OlaServer is the main OLA Server class
 * Copyright (C) 2005-2008 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <vector>

#include "common/protocol/Ola.pb.h"
#include "common/rpc/StreamRpcChannel.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "olad/Client.h"
#include "olad/DeviceManager.h"
#include "olad/OlaServer.h"
#include "olad/OlaServerServiceImpl.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/PluginManager.h"
#include "olad/Port.h"
#include "olad/PortPatcher.h"
#include "olad/Preferences.h"
#include "olad/Universe.h"
#include "olad/UniverseStore.h"

#ifdef HAVE_LIBMICROHTTPD
#include "olad/OlaHttpServer.h"
#endif

namespace ola {

using ola::rpc::StreamRpcChannel;
using std::pair;

const char OlaServer::UNIVERSE_PREFERENCES[] = "universe";
const char OlaServer::K_CLIENT_VAR[] = "clients-connected";
const unsigned int OlaServer::K_GARBAGE_COLLECTOR_TIMEOUT_MS = 5000;


/*
 * Create a new OlaServer
 * @param factory the factory to use to create OlaService objects
 * @param m_plugin_loader the loader to use for the plugins
 * @param socket the socket to listen on for new connections
 */
OlaServer::OlaServer(OlaServerServiceImplFactory *factory,
                     const vector<PluginLoader*> &plugin_loaders,
                     PreferencesFactory *preferences_factory,
                     ola::network::SelectServer *select_server,
                     ola_server_options *ola_options,
                     ola::network::AcceptingSocket *socket,
                     ExportMap *export_map)
    : m_service_factory(factory),
      m_plugin_loaders(plugin_loaders),
      m_ss(select_server),
      m_accepting_socket(socket),
      m_device_manager(NULL),
      m_plugin_adaptor(NULL),
      m_preferences_factory(preferences_factory),
      m_universe_preferences(NULL),
      m_universe_store(NULL),
      m_export_map(export_map),
      m_port_patcher(NULL),
      m_init_run(false),
      m_free_export_map(false),
      m_garbage_collect_timeout(ola::network::INVALID_TIMEOUT),
      m_httpd(NULL),
      m_options(*ola_options) {
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
OlaServer::~OlaServer() {
#ifdef HAVE_LIBMICROHTTPD
  if (m_httpd) {
    m_httpd->Stop();
    delete m_httpd;
    m_httpd = NULL;
  }
#endif

  if (m_garbage_collect_timeout != ola::network::INVALID_TIMEOUT)
    m_ss->RemoveTimeout(m_garbage_collect_timeout);

  StopPlugins();

  map<int, OlaServerServiceImpl*>::iterator iter;
  for (iter = m_sd_to_service.begin(); iter != m_sd_to_service.end(); ++iter) {
    CleanupConnection(iter->second);
    // TODO(simon): close the socket here

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

  delete m_port_patcher;
  delete m_plugin_adaptor;
  delete m_device_manager;
  delete m_plugin_manager;

  if (m_free_export_map)
    delete m_export_map;
}


/*
 * Initialise the server
 * * @return true on success, false on failure
 */
bool OlaServer::Init() {
  if (m_init_run)
    return false;

  if (!m_service_factory || !m_ss)
    return false;

  // TODO(simon): run without preferences & PluginLoader
  if (!m_plugin_loaders.size() || !m_preferences_factory)
    return false;

  if (m_accepting_socket) {
    if (!m_accepting_socket->Listen())
      return false;
    m_accepting_socket->SetOnData(
      ola::NewClosure(this, &OlaServer::AcceptNewConnection,
                      m_accepting_socket));
    m_ss->AddSocket(m_accepting_socket);
  }

  signal(SIGPIPE, SIG_IGN);

  m_universe_preferences = m_preferences_factory->NewPreference(
      UNIVERSE_PREFERENCES);
  m_universe_preferences->Load();
  m_universe_store = new UniverseStore(m_universe_preferences, m_export_map);

  m_port_patcher = new PortPatcher(m_universe_store);

  // setup the objects
  m_device_manager = new DeviceManager(m_preferences_factory, m_port_patcher);
  m_plugin_adaptor = new PluginAdaptor(m_device_manager,
                                       m_ss,
                                       m_preferences_factory);

  m_plugin_manager = new PluginManager(m_plugin_loaders, m_plugin_adaptor);

  if (!m_universe_store || !m_device_manager || !m_plugin_adaptor ||
      !m_port_patcher || !m_plugin_manager) {
    delete m_plugin_adaptor;
    delete m_device_manager;
    delete m_port_patcher;
    delete m_universe_store;
    delete m_plugin_manager;
    return false;
  }

  m_plugin_manager->LoadAll();

#ifdef HAVE_LIBMICROHTTPD
  if (m_options.http_enable) {
    m_httpd = new OlaHttpServer(m_export_map,
                                m_ss,
                                m_universe_store,
                                m_plugin_manager,
                                m_device_manager,
                                m_port_patcher,
                                m_options.http_port,
                                m_options.http_enable_quit,
                                m_options.http_data_dir);
    m_httpd->Start();
  }
#endif

  m_garbage_collect_timeout = m_ss->RegisterRepeatingTimeout(
      K_GARBAGE_COLLECTOR_TIMEOUT_MS,
      ola::NewClosure(this, &OlaServer::GarbageCollect));

  m_init_run = true;
  return true;
}


/*
 * Reload all plugins
 */
void OlaServer::ReloadPlugins() {
  OLA_INFO << "Reloading plugins";
  StopPlugins();
  m_plugin_manager->LoadAll();
}


/*
 * Add a new ConnectedSocket to this Server.
 * @param accepting_socket the AcceptingSocket with the new connection pending.
 */
int OlaServer::AcceptNewConnection(
    ola::network::AcceptingSocket *accepting_socket) {
  ola::network::ConnectedSocket *socket = accepting_socket->Accept();

  if (!socket)
    return 0;

  return NewConnection(socket) ? 0 : -1;
}


/*
 * Add a new ConnectedSocket to this Server.
 * @param socket the new ConnectedSocket
 */
bool OlaServer::NewConnection(ola::network::ConnectedSocket *socket) {
  if (!socket)
    return -1;

  StreamRpcChannel *channel = new StreamRpcChannel(NULL, socket);
  socket->SetOnClose(NewSingleClosure(this, &OlaServer::SocketClosed, socket));
  OlaClientService_Stub *stub = new OlaClientService_Stub(channel);
  Client *client = new Client(stub);
  OlaServerServiceImpl *service = m_service_factory->New(m_universe_store,
                                                         m_device_manager,
                                                         m_plugin_manager,
                                                         client,
                                                         m_export_map,
                                                         m_port_patcher);
  channel->SetService(service);

  map<int, OlaServerServiceImpl*>::const_iterator iter;
  iter = m_sd_to_service.find(socket->ReadDescriptor());

  if (iter != m_sd_to_service.end())
    OLA_INFO << "New socket but the client already exists!";

  pair<int, OlaServerServiceImpl*> pair(socket->ReadDescriptor(), service);
  m_sd_to_service.insert(pair);

  m_ss->AddSocket(socket, true);
  IntegerVariable *var = m_export_map->GetIntegerVar(K_CLIENT_VAR);
  var->Increment();
  return 0;
}


/*
 * Called when a socket is closed
 */
int OlaServer::SocketClosed(ola::network::ConnectedSocket *socket) {
  map<int, OlaServerServiceImpl*>::iterator iter;
  iter = m_sd_to_service.find(socket->ReadDescriptor());

  if (iter == m_sd_to_service.end())
    OLA_INFO << "A socket was closed but we didn't find the client";

  IntegerVariable *var = m_export_map->GetIntegerVar(K_CLIENT_VAR);
  var->Decrement();
  CleanupConnection(iter->second);
  m_sd_to_service.erase(iter);
  return 0;
}


/*
 * Run the garbage collector
 */
int OlaServer::GarbageCollect() {
  OLA_INFO << "Garbage collecting";
  m_universe_store->GarbageCollectUniverses();
  return 0;
}


/*
 * Stop and unload all the plugins
 */
void OlaServer::StopPlugins() {
  m_plugin_manager->UnloadAll();
  if (m_device_manager) {
    if (m_device_manager->DeviceCount()) {
      OLA_WARN << "Some devices failed to unload, we're probably leaking "
        << "memory now";
    }
    m_device_manager->UnregisterAllDevices();
  }
}


/*
 * Cleanup everything related to a client connection
 */
void OlaServer::CleanupConnection(OlaServerServiceImpl *service) {
  Client *client = service->GetClient();

  vector<Universe*> universe_list;
  m_universe_store->GetList(&universe_list);
  vector<Universe*>::iterator uni_iter;

  // O(universes * clients). Clean this up sometime.
  for (uni_iter = universe_list.begin();
       uni_iter != universe_list.end();
       ++uni_iter) {
    (*uni_iter)->RemoveSourceClient(client);
    (*uni_iter)->RemoveSinkClient(client);
  }
  delete client->Stub()->channel();
  delete client->Stub();
  delete client;
  delete service;
}
}  // ola
