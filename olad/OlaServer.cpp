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
#include <memory>
#include <utility>
#include <vector>

#include "ola/network/Socket.h"
#include "common/protocol/Ola.pb.h"
#include "common/rpc/StreamRpcChannel.h"
#include "ola/BaseTypes.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/network/InterfacePicker.h"
#include "ola/rdm/PidStore.h"
#include "ola/rdm/UID.h"
#include "ola/stl/STLUtils.h"
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
#include "olad/OladHTTPServer.h"
#endif

namespace ola {

using ola::rdm::RootPidStore;
using ola::rpc::StreamRpcChannel;
using std::auto_ptr;
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
                     const Options &ola_options,
                     ola::network::TCPAcceptingSocket *socket,
                     ExportMap *export_map)
    : m_service_factory(factory),
      m_plugin_loaders(plugin_loaders),
      m_ss(select_server),
      m_tcp_socket_factory(
          ola::NewCallback(this, &OlaServer::NewTCPConnection)),
      m_accepting_socket(socket),
      m_export_map(export_map),
      m_preferences_factory(preferences_factory),
      m_universe_preferences(NULL),
      m_housekeeping_timeout(ola::thread::INVALID_TIMEOUT),
      m_options(ola_options),
      m_default_uid(OPEN_LIGHTING_ESTA_CODE, 0) {
  if (!m_export_map) {
    m_our_export_map.reset(new ExportMap());
    m_export_map = m_our_export_map.get();
  }

  m_export_map->GetIntegerVar(K_CLIENT_VAR);
}


/*
 * Shutdown the server
 */
OlaServer::~OlaServer() {
#ifdef HAVE_LIBMICROHTTPD
  if (m_httpd.get()) {
    m_httpd->Stop();
    m_httpd.reset();
  }
#endif

  if (m_housekeeping_timeout != ola::thread::INVALID_TIMEOUT)
    m_ss->RemoveTimeout(m_housekeeping_timeout);

  StopPlugins();

  ClientMap::iterator iter = m_sd_to_service.begin();
  for (; iter != m_sd_to_service.end(); ++iter) {
    CleanupConnection(iter->second);
  }

  m_broker.reset();
  m_port_broker.reset();

  if (m_accepting_socket && m_accepting_socket->ValidReadDescriptor())
    m_ss->RemoveReadDescriptor(m_accepting_socket);

  if (m_universe_store.get()) {
    m_universe_store->DeleteAll();
    m_universe_store.reset();
  }

  if (m_universe_preferences) {
    m_universe_preferences->Save();
  }

  m_port_manager.reset();
  m_plugin_adaptor.reset();
  m_device_manager.reset();
  m_plugin_manager.reset();
  m_service_impl.reset();
}


/*
 * Initialise the server
 * * @return true on success, false on failure
 */
bool OlaServer::Init() {
  if (m_service_impl.get())
    return false;

  if (!m_service_factory || !m_ss)
    return false;

  // TODO(simon): run without preferences & PluginLoader
  if (m_plugin_loaders.empty() || !m_preferences_factory)
    return false;

  const RootPidStore* pid_store =
    RootPidStore::LoadFromDirectory(m_options.pid_data_dir);
  if (!pid_store)
    OLA_WARN << "No PID definitions loaded";

  UpdatePidStore(pid_store);

  if (m_accepting_socket) {
    m_accepting_socket->SetFactory(&m_tcp_socket_factory);
    m_ss->AddReadDescriptor(m_accepting_socket);
  }

#ifndef WIN32
  signal(SIGPIPE, SIG_IGN);
#endif

  // fetch the interface info
  ola::network::Interface iface;
  {
    auto_ptr<ola::network::InterfacePicker> picker(
      ola::network::InterfacePicker::NewPicker());
    if (!picker->ChooseInterface(&iface, m_options.interface)) {
      OLA_WARN << "No network interface found";
    } else {
      // default to using the ip as a id
      m_default_uid = ola::rdm::UID(OPEN_LIGHTING_ESTA_CODE,
                                    iface.ip_address.AsInt());
    }
  }
  m_export_map->GetStringVar(K_UID_VAR)->Set(m_default_uid.ToString());
  OLA_INFO << "Server UID is " << m_default_uid;

  m_universe_preferences = m_preferences_factory->NewPreference(
      UNIVERSE_PREFERENCES);
  m_universe_preferences->Load();
  m_universe_store.reset(
      new UniverseStore(m_universe_preferences, m_export_map));

  m_port_broker.reset(new PortBroker());
  m_port_manager.reset(
      new PortManager(m_universe_store.get(), m_port_broker.get()));
  m_broker.reset(new ClientBroker());

  // setup the objects
  m_device_manager.reset(
      new DeviceManager(m_preferences_factory, m_port_manager.get()));
  m_plugin_adaptor.reset(
      new PluginAdaptor(m_device_manager.get(), m_ss, m_export_map,
                        m_preferences_factory, m_port_broker.get()));

  m_plugin_manager.reset(
    new PluginManager(m_plugin_loaders, m_plugin_adaptor.get()));
  m_service_impl.reset(new OlaServerServiceImpl(
      m_universe_store.get(),
      m_device_manager.get(),
      m_plugin_manager.get(),
      m_export_map,
      m_port_manager.get(),
      m_broker.get(),
      m_ss->WakeUpTime(),
      m_default_uid));

  // The plugin load procedure can take a while so we run it in the main loop.
  m_ss->Execute(
      ola::NewSingleCallback(m_plugin_manager.get(), &PluginManager::LoadAll));

#ifdef HAVE_LIBMICROHTTPD
  if (!StartHttpServer(iface))
    OLA_WARN << "Failed to start the HTTP server.";
#endif

  m_housekeeping_timeout = m_ss->RegisterRepeatingTimeout(
      K_HOUSEKEEPING_TIMEOUT_MS,
      ola::NewCallback(this, &OlaServer::RunHousekeeping));

  return true;
}


/*
 * Reload all plugins, this can be called from a separate thread or in an
 * interrupt handler.
 */
void OlaServer::ReloadPlugins() {
  m_ss->Execute(NewCallback(this, &OlaServer::ReloadPluginsInternal));
}


/*
 * Reload the pid store.
 */
void OlaServer::ReloadPidStore() {
  // We load the pids in this thread, and then hand the RootPidStore over to
  // the main thread. This avoids doing disk I/O in the network thread.
  const RootPidStore* pid_store = RootPidStore::LoadFromDirectory(
      m_options.pid_data_dir);
  if (!pid_store)
    return;

  m_ss->Execute(NewCallback(this, &OlaServer::UpdatePidStore, pid_store));
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
void OlaServer::NewTCPConnection(ola::network::TCPSocket *socket) {
  if (!socket)
    return;
  InternalNewConnection(socket);
}


/*
 * Called when a socket is closed
 */
void OlaServer::ChannelClosed(int read_descriptor) {
  ClientEntry client_entry;
  bool found = STLLookupAndRemove(&m_sd_to_service, read_descriptor,
                                  &client_entry);
  if (found) {
    (*m_export_map->GetIntegerVar(K_CLIENT_VAR))--;
    m_ss->Execute(NewSingleCallback(this, &OlaServer::CleanupConnection,
                                    client_entry));
  } else {
    OLA_WARN << "A socket was closed but we didn't find the client";
  }
}


/*
 * Run the garbage collector
 */
bool OlaServer::RunHousekeeping() {
  OLA_DEBUG << "Garbage collecting";
  m_universe_store->GarbageCollectUniverses();

  // Give the universes an opportunity to run discovery
  vector<Universe*> universes;
  m_universe_store->GetList(&universes);

  vector<Universe*>::iterator iter = universes.begin();
  const TimeStamp *now = m_ss->WakeUpTime();
  for (; iter != universes.end(); ++iter) {
    if ((*iter)->RDMDiscoveryInterval().Seconds() &&
        *now - (*iter)->LastRDMDiscovery() > (*iter)->RDMDiscoveryInterval()) {
      // run incremental discovery
      (*iter)->RunRDMDiscovery(NULL, false);
    }
    (*iter)->CleanStaleSourceClients();
  }
  return true;
}


/*
 * Setup the HTTP server if required.
 * @param interface the primary interface that the server is using.
 */
#ifdef HAVE_LIBMICROHTTPD
bool OlaServer::StartHttpServer(const ola::network::Interface &iface) {
  if (!m_options.http_enable)
    return true;

  // create a pipe for the http server to communicate with the main
  // server on.
  auto_ptr<ola::io::PipeDescriptor> pipe_descriptor(
      new ola::io::PipeDescriptor());
  if (!pipe_descriptor->Init()) {
    return false;
  }

  // ownership of the pipe_descriptor is transferred here.
  OladHTTPServer::OladHTTPServerOptions options;
  options.port = m_options.http_port ? m_options.http_port : DEFAULT_HTTP_PORT;
  options.data_dir = (m_options.http_data_dir.empty() ? HTTP_DATA_DIR :
                      m_options.http_data_dir);
  options.enable_quit = m_options.http_enable_quit;

  auto_ptr<OladHTTPServer> httpd(
      new OladHTTPServer(m_export_map, options,
                         pipe_descriptor->OppositeEnd(),
                         this, iface));

  if (httpd->Init()) {
    httpd->Start();
    // register the pipe descriptor as a client
    InternalNewConnection(pipe_descriptor.release());
    m_httpd.reset(httpd.release());
    return true;
  } else {
    pipe_descriptor->Close();
    return false;
  }
}
#endif


/*
 * Stop and unload all the plugins
 */
void OlaServer::StopPlugins() {
  if (m_plugin_manager.get())
    m_plugin_manager->UnloadAll();
  if (m_device_manager.get()) {
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
  channel->SetChannelCloseHandler(
      NewSingleCallback(this, &OlaServer::ChannelClosed,
                        socket->ReadDescriptor()));
  OlaClientService_Stub *stub = new OlaClientService_Stub(channel);
  Client *client = new Client(stub);
  OlaClientService *service = m_service_factory->New(
      client, m_service_impl.get());
  m_broker->AddClient(client);
  channel->SetService(service);

  ClientEntry client_entry = {socket, service};
  bool replaced = STLReplace(&m_sd_to_service, socket->ReadDescriptor(),
                             client_entry);
  if (replaced) {
    OLA_WARN << "New socket but the client already exists!";
  } else {
    (*m_export_map->GetIntegerVar(K_CLIENT_VAR))++;
  }

  // This hands off socket ownership to the select server
  m_ss->AddReadDescriptor(socket);
}


/*
 * Cleanup everything related to a client connection
 */
void OlaServer::CleanupConnection(ClientEntry client_entry) {
  Client *client = client_entry.client_service->GetClient();
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
  delete client_entry.client_service;
  m_ss->RemoveReadDescriptor(client_entry.client_descriptor);
  delete client_entry.client_descriptor;
}

/**
 * Reload the plugins. Called from the SelectServer thread.
 */
void OlaServer::ReloadPluginsInternal() {
  OLA_INFO << "Reloading plugins";
  StopPlugins();
  m_plugin_manager->LoadAll();
}

/**
 * Update the Pid store with the new values.
 */
void OlaServer::UpdatePidStore(const RootPidStore *pid_store) {
  OLA_INFO << "Updated PID definitions.";
#ifdef HAVE_LIBMICROHTTPD
  if (m_httpd.get()) {
    m_httpd->SetPidStore(pid_store);
  }
#endif

  m_pid_store.reset(pid_store);
}
}  // namespace ola
