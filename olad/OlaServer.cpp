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
#include <utility>
#include <vector>

#include "ola/network/Socket.h"
#include "common/protocol/Ola.pb.h"
#include "common/rpc/StreamRpcChannel.h"
#include "ola/BaseTypes.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/network/InterfacePicker.h"
#include "ola/rdm/UID.h"
#include "olad/Client.h"
#include "olad/DeviceManager.h"
#include "olad/ClientBroker.h"
#include "olad/OlaServer.h"
#include "olad/OlaServerServiceImpl.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/PluginManager.h"
#include "olad/Port.h"
#include "olad/PortBroker.h"
#include "olad/PortManager.h"
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
const char OlaServer::K_UID_VAR[] = "server-uid";
const unsigned int OlaServer::K_HOUSEKEEPING_TIMEOUT_MS = 10000;


/*
 * Create a new OlaServer
 * @param factory the factory to use to create OlaService objects
 * @param m_plugin_loader the loader to use for the plugins
 * @param socket the socket to listen on for new connections
 */
OlaServer::OlaServer(OlaClientServiceFactory *factory,
                     const vector<PluginLoader*> &plugin_loaders,
                     PreferencesFactory *preferences_factory,
                     ola::io::SelectServer *select_server,
                     ola_server_options *ola_options,
                     ola::network::TcpAcceptingSocket *socket,
                     ExportMap *export_map)
    : m_service_factory(factory),
      m_plugin_loaders(plugin_loaders),
      m_ss(select_server),
      m_tcp_socket_factory(ola::NewCallback(this, &OlaServer::NewTCPConnection)),
      m_accepting_socket(socket),
      m_device_manager(NULL),
      m_plugin_manager(NULL),
      m_plugin_adaptor(NULL),
      m_preferences_factory(preferences_factory),
      m_universe_preferences(NULL),
      m_universe_store(NULL),
      m_export_map(export_map),
      m_port_manager(NULL),
      m_service_impl(NULL),
      m_broker(NULL),
      m_port_broker(NULL),
      m_reload_plugins(false),
      m_init_run(false),
      m_free_export_map(false),
      m_housekeeping_timeout(ola::thread::INVALID_TIMEOUT),
      m_httpd(NULL),
      m_options(*ola_options),
      m_default_uid(OPEN_LIGHTING_ESTA_CODE, 0) {
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

  if (m_housekeeping_timeout != ola::thread::INVALID_TIMEOUT)
    m_ss->RemoveTimeout(m_housekeeping_timeout);

  StopPlugins();

  map<int, OlaClientService*>::iterator iter;
  for (iter = m_sd_to_service.begin(); iter != m_sd_to_service.end(); ++iter) {
    CleanupConnection(iter->second);
    // TODO(simon): close the socket here

    /*Socket *socket = ;
    m_ss->RemoveReadDescriptor(socket);
    socket->Close();
    */
  }

  if (m_broker)
    delete m_broker;

  if (m_port_broker)
    delete m_port_broker;

  if (m_accepting_socket &&
      m_accepting_socket->ValidReadDescriptor())
    m_ss->RemoveReadDescriptor(m_accepting_socket);

  if (m_universe_store) {
    m_universe_store->DeleteAll();
    delete m_universe_store;
  }

  if (m_universe_preferences) {
    m_universe_preferences->Save();
  }

  delete m_port_manager;
  delete m_plugin_adaptor;
  delete m_device_manager;
  delete m_plugin_manager;
  delete m_service_impl;

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
  if (m_plugin_loaders.empty() || !m_preferences_factory)
    return false;

  if (m_accepting_socket) {
    m_accepting_socket->SetFactory(&m_tcp_socket_factory);
    m_ss->AddReadDescriptor(m_accepting_socket);
  }

#ifndef WIN32
  signal(SIGPIPE, SIG_IGN);
#endif

  // fetch the interface info
  ola::network::Interface iface;
  ola::network::InterfacePicker *picker =
    ola::network::InterfacePicker::NewPicker();
  if (!picker->ChooseInterface(&iface, m_options.interface)) {
    OLA_WARN << "No network interface found";
  } else {
    // default to using the ip as a id
    m_default_uid = ola::rdm::UID(OPEN_LIGHTING_ESTA_CODE,
                                  iface.ip_address.AsInt());
  }
  delete picker;
  m_export_map->GetStringVar(K_UID_VAR)->Set(m_default_uid.ToString());
  OLA_INFO << "Server UID is " << m_default_uid;

  m_universe_preferences = m_preferences_factory->NewPreference(
      UNIVERSE_PREFERENCES);
  m_universe_preferences->Load();
  m_universe_store = new UniverseStore(m_universe_preferences, m_export_map);

  m_port_broker = new PortBroker();
  m_port_manager = new PortManager(m_universe_store, m_port_broker);
  m_broker = new ClientBroker();

  // setup the objects
  m_device_manager = new DeviceManager(m_preferences_factory, m_port_manager);
  m_plugin_adaptor = new PluginAdaptor(m_device_manager,
                                       m_ss,
                                       m_preferences_factory,
                                       m_port_broker);

  m_plugin_manager = new PluginManager(m_plugin_loaders, m_plugin_adaptor);
  m_service_impl = new OlaServerServiceImpl(
      m_universe_store,
      m_device_manager,
      m_plugin_manager,
      m_export_map,
      m_port_manager,
      m_broker,
      m_ss->WakeUpTime(),
      m_default_uid);

  if (!m_port_broker || !m_universe_store || !m_device_manager ||
      !m_plugin_adaptor || !m_port_manager || !m_plugin_manager || !m_broker ||
      !m_service_impl) {
    delete m_plugin_adaptor;
    delete m_device_manager;
    delete m_port_manager;
    delete m_universe_store;
    delete m_plugin_manager;
    return false;
  }

  // The plugin load procedure can take a while so we run it in the main loop.
  m_ss->Execute(
      ola::NewSingleCallback(m_plugin_manager, &PluginManager::LoadAll));

#ifdef HAVE_LIBMICROHTTPD
  if (!StartHttpServer(iface))
    OLA_WARN << "Failed to start the HTTP server.";
#endif

  m_housekeeping_timeout = m_ss->RegisterRepeatingTimeout(
      K_HOUSEKEEPING_TIMEOUT_MS,
      ola::NewCallback(this, &OlaServer::RunHousekeeping));
  m_ss->RunInLoop(ola::NewCallback(this, &OlaServer::CheckForReload));

  m_init_run = true;
  return true;
}


/*
 * Reload all plugins, this can be called from a separate thread or in an
 * interrupt handler.
 */
void OlaServer::ReloadPlugins() {
  m_reload_plugins = true;
}


/*
 * Add a new ConnectedDescriptor to this Server.
 * @param socket the new ConnectedDescriptor
 */
void OlaServer::NewConnection(ola::io::ConnectedDescriptor *descriptor) {
  if (!descriptor)
    return;
  InternalNewConnection(descriptor);
}


/*
 * Add a new ConnectedDescriptor to this Server.
 * @param socket the new ConnectedDescriptor
 */
void OlaServer::NewTCPConnection(ola::network::TcpSocket *socket) {
  if (!socket)
    return;
  InternalNewConnection(socket);
}


/*
 * Called when a socket is closed
 */
void OlaServer::SocketClosed(ola::io::ConnectedDescriptor *socket) {
  map<int, OlaClientService*>::iterator iter;
  iter = m_sd_to_service.find(socket->ReadDescriptor());

  if (iter == m_sd_to_service.end()) {
    OLA_WARN << "A socket was closed but we didn't find the client";
    return;
  }

  (*m_export_map->GetIntegerVar(K_CLIENT_VAR))--;
  CleanupConnection(iter->second);
  m_sd_to_service.erase(iter);
}


/*
 * Run the garbage collector
 */
bool OlaServer::RunHousekeeping() {
  OLA_DEBUG << "Garbage collecting";
  m_universe_store->GarbageCollectUniverses();
  return true;
}


/*
 * Called once per loop iteration
 */
void OlaServer::CheckForReload() {
  if (m_reload_plugins) {
    m_reload_plugins = false;
    OLA_INFO << "Reloading plugins";
    StopPlugins();
    m_plugin_manager->LoadAll();
  }
}


/*
 * Setup the HTTP server if required.
 * @param interface the primary interface that the server is using.
 */
#ifdef HAVE_LIBMICROHTTPD
bool OlaServer::StartHttpServer(const ola::network::Interface &iface) {
  if (!m_options.http_enable)
    return true;

  // create a pipe socket for the http server to communicate with the main
  // server on.
  ola::io::PipeDescriptor *pipe_descriptor = new ola::io::PipeDescriptor();
  if (!pipe_descriptor->Init()) {
    delete pipe_descriptor;
    return false;
  }

  // ownership of the pipe_descriptor is transferred here.
  m_httpd = new OlaHttpServer(m_export_map,
                              pipe_descriptor->OppositeEnd(),
                              this,
                              m_options.http_port,
                              m_options.http_enable_quit,
                              m_options.http_data_dir,
                              iface);

  if (m_httpd->Init()) {
    m_httpd->Start();
    // register the pipe descriptor as a client
    InternalNewConnection(pipe_descriptor);
    return true;
  } else {
    pipe_descriptor->Close();
    delete pipe_descriptor;
    delete m_httpd;
    m_httpd = NULL;
    return false;
  }
}
#endif


/*
 * Stop and unload all the plugins
 */
void OlaServer::StopPlugins() {
  if (m_plugin_manager)
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
 * Add a new ConnectedDescriptor to this Server.
 * @param socket the new ConnectedDescriptor
 */
void OlaServer::InternalNewConnection(
    ola::io::ConnectedDescriptor *socket) {
  StreamRpcChannel *channel = new StreamRpcChannel(NULL, socket, m_export_map);
  socket->SetOnClose(
      NewSingleCallback(this, &OlaServer::SocketClosed, socket));
  OlaClientService_Stub *stub = new OlaClientService_Stub(channel);
  Client *client = new Client(stub);
  OlaClientService *service = m_service_factory->New(client, m_service_impl);
  m_broker->AddClient(client);
  channel->SetService(service);

  map<int, OlaClientService*>::const_iterator iter;
  iter = m_sd_to_service.find(socket->ReadDescriptor());

  if (iter != m_sd_to_service.end())
    OLA_INFO << "New socket but the client already exists!";

  pair<int, OlaClientService*> pair(socket->ReadDescriptor(), service);
  m_sd_to_service.insert(pair);

  // This hands off ownership to the select server
  m_ss->AddReadDescriptor(socket, true);
  (*m_export_map->GetIntegerVar(K_CLIENT_VAR))++;
}


/*
 * Cleanup everything related to a client connection
 */
void OlaServer::CleanupConnection(OlaClientService *service) {
  Client *client = service->GetClient();
  m_broker->RemoveClient(client);

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
