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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * OlaServer.cpp
 * OlaServer is the main OLA Server class
 * Copyright (C) 2005 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <utility>
#include <vector>

#include "common/protocol/Ola.pb.h"
#include "common/rpc/RpcChannel.h"
#include "common/rpc/RpcServer.h"
#include "common/rpc/RpcSession.h"
#include "ola/Constants.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/Socket.h"
#include "ola/rdm/PidStore.h"
#include "ola/rdm/UID.h"
#include "ola/stl/STLUtils.h"
#include "olad/ClientBroker.h"
#include "olad/DiscoveryAgent.h"
#include "olad/OlaServer.h"
#include "olad/OlaServerServiceImpl.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/PluginManager.h"
#include "olad/Port.h"
#include "olad/PortBroker.h"
#include "olad/Preferences.h"
#include "olad/Universe.h"
#include "olad/plugin_api/Client.h"
#include "olad/plugin_api/DeviceManager.h"
#include "olad/plugin_api/PortManager.h"
#include "olad/plugin_api/UniverseStore.h"

#ifdef HAVE_LIBMICROHTTPD
#include "olad/OladHTTPServer.h"
#endif  // HAVE_LIBMICROHTTPD

DEFINE_s_uint16(rpc_port, r, ola::OlaServer::DEFAULT_RPC_PORT,
                "The port to listen for RPCs on. Defaults to 9010.");
DEFINE_default_bool(register_with_dns_sd, true,
                    "Don't register the web service using DNS-SD (Bonjour).");

namespace ola {

using ola::proto::OlaClientService_Stub;
using ola::rdm::RootPidStore;
using ola::rpc::RpcChannel;
using ola::rpc::RpcSession;
using ola::rpc::RpcServer;
using std::auto_ptr;
using std::pair;
using std::vector;

const char OlaServer::INSTANCE_NAME_KEY[] = "instance-name";
const char OlaServer::K_INSTANCE_NAME_VAR[] = "server-instance-name";
const char OlaServer::K_UID_VAR[] = "server-uid";
const char OlaServer::SERVER_PREFERENCES[] = "server";
const char OlaServer::UNIVERSE_PREFERENCES[] = "universe";
// The Bonjour API expects <service>[,<sub-type>] so we use that form here.
const char OlaServer::K_DISCOVERY_SERVICE_TYPE[] = "_http._tcp,_ola";
const unsigned int OlaServer::K_HOUSEKEEPING_TIMEOUT_MS = 10000;

OlaServer::OlaServer(const vector<PluginLoader*> &plugin_loaders,
                     PreferencesFactory *preferences_factory,
                     ola::io::SelectServer *select_server,
                     const Options &ola_options,
                     ola::network::TCPAcceptingSocket *socket,
                     ExportMap *export_map)
    : m_options(ola_options),
      m_plugin_loaders(plugin_loaders),
      m_preferences_factory(preferences_factory),
      m_ss(select_server),
      m_accepting_socket(socket),
      m_export_map(export_map),
      m_default_uid(OPEN_LIGHTING_ESTA_CODE, 0),
      m_server_preferences(NULL),
      m_universe_preferences(NULL),
      m_housekeeping_timeout(ola::thread::INVALID_TIMEOUT) {
  if (!m_export_map) {
    m_our_export_map.reset(new ExportMap());
    m_export_map = m_our_export_map.get();
  }
}

OlaServer::~OlaServer() {
  m_ss->DrainCallbacks();

#ifdef HAVE_LIBMICROHTTPD
  if (m_httpd.get()) {
    m_httpd->Stop();
    m_httpd.reset();
  }
#endif  // HAVE_LIBMICROHTTPD

  // Order is important during shutdown.
  // Shutdown the RPC server first since it depends on almost everything else.
  m_rpc_server.reset();

  if (m_housekeeping_timeout != ola::thread::INVALID_TIMEOUT) {
    m_ss->RemoveTimeout(m_housekeeping_timeout);
  }

  StopPlugins();

  m_broker.reset();
  m_port_broker.reset();

  if (m_universe_store.get()) {
    m_universe_store->DeleteAll();
    m_universe_store.reset();
  }

  if (m_server_preferences) {
    m_server_preferences->Save();
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

bool OlaServer::Init() {
  if (m_service_impl.get()) {
    return false;
  }

  if (!m_ss) {
    return false;
  }

  // TODO(simon): run without preferences & PluginLoader
  if (m_plugin_loaders.empty() || !m_preferences_factory) {
    return false;
  }

  auto_ptr<const RootPidStore> pid_store(
      RootPidStore::LoadFromDirectory(m_options.pid_data_dir));
  if (!pid_store.get()) {
    OLA_WARN << "No PID definitions loaded";
  }

#ifndef _WIN32
  signal(SIGPIPE, SIG_IGN);
#endif  // _WIN32

  // fetch the interface info
  ola::network::Interface iface;
  {
    auto_ptr<ola::network::InterfacePicker> picker(
        ola::network::InterfacePicker::NewPicker());
    if (!picker->ChooseInterface(&iface, m_options.network_interface)) {
      OLA_WARN << "No network interface found";
    } else {
      // default to using the ip as an id
      m_default_uid = ola::rdm::UID(OPEN_LIGHTING_ESTA_CODE,
                                    iface.ip_address.AsInt());
    }
  }
  m_export_map->GetStringVar(K_UID_VAR)->Set(m_default_uid.ToString());
  OLA_INFO << "Server UID is " << m_default_uid;

  m_server_preferences = m_preferences_factory->NewPreference(
      SERVER_PREFERENCES);
  m_server_preferences->Load();
  if (m_server_preferences->SetDefaultValue(INSTANCE_NAME_KEY,
                                            StringValidator(),
                                            OLA_DEFAULT_INSTANCE_NAME)) {
    m_server_preferences->Save();
  }
  m_instance_name = m_server_preferences->GetValue(INSTANCE_NAME_KEY);
  m_export_map->GetStringVar(K_INSTANCE_NAME_VAR)->Set(m_instance_name);
  OLA_INFO << "Server instance name is " << m_instance_name;

  Preferences *universe_preferences = m_preferences_factory->NewPreference(
      UNIVERSE_PREFERENCES);
  universe_preferences->Load();

  auto_ptr<UniverseStore> universe_store(
      new UniverseStore(universe_preferences, m_export_map));

  auto_ptr<PortBroker> port_broker(new PortBroker());

  auto_ptr<PortManager> port_manager(
      new PortManager(universe_store.get(), port_broker.get()));

  auto_ptr<ClientBroker> broker(new ClientBroker());

  auto_ptr<DeviceManager> device_manager(
      new DeviceManager(m_preferences_factory, port_manager.get()));

  auto_ptr<PluginAdaptor> plugin_adaptor(
      new PluginAdaptor(device_manager.get(), m_ss, m_export_map,
                        m_preferences_factory, port_broker.get(),
                        &m_instance_name));

  auto_ptr<PluginManager> plugin_manager(
    new PluginManager(m_plugin_loaders, plugin_adaptor.get()));

  auto_ptr<OlaServerServiceImpl> service_impl(new OlaServerServiceImpl(
      universe_store.get(),
      device_manager.get(),
      plugin_manager.get(),
      port_manager.get(),
      broker.get(),
      m_ss->WakeUpTime(),
      NewCallback(this, &OlaServer::ReloadPluginsInternal)));

  // Initialize the RPC server.
  RpcServer::Options rpc_options;
  rpc_options.listen_socket = m_accepting_socket;
  rpc_options.listen_port = FLAGS_rpc_port;
  rpc_options.export_map = m_export_map;

  auto_ptr<ola::rpc::RpcServer> rpc_server(
      new RpcServer(m_ss, service_impl.get(), this, rpc_options));

  if (!rpc_server->Init()) {
    OLA_WARN << "Failed to init RPC server";
    return false;
  }

  // Discovery
  auto_ptr<DiscoveryAgentInterface> discovery_agent;
  if (FLAGS_register_with_dns_sd) {
    DiscoveryAgentFactory discovery_agent_factory;
    discovery_agent.reset(discovery_agent_factory.New());
    if (discovery_agent.get()) {
      if (!discovery_agent->Init()) {
        OLA_WARN << "Failed to Init DiscoveryAgent";
        return false;
      }
    }
  }

  bool web_server_started = false;

  // Initializing the web server causes a call to NewClient. We need to have
  // the broker in place for the call, otherwise we'll segfault.
  m_broker.reset(broker.release());

#ifdef HAVE_LIBMICROHTTPD
  if (m_options.http_enable) {
    if (StartHttpServer(rpc_server.get(), iface)) {
      web_server_started = true;
    } else {
      OLA_WARN << "Failed to start the HTTP server.";
      m_broker.reset();
      return false;
    }
  }
#endif  // HAVE_LIBMICROHTTPD

  if (web_server_started && discovery_agent.get()) {
    DiscoveryAgentInterface::RegisterOptions options;
    options.txt_data["path"] = "/";
    discovery_agent->RegisterService(
        m_instance_name,
        K_DISCOVERY_SERVICE_TYPE,
        m_options.http_port,
        options);
  }

  // Ok, we've created and initialized everything correctly by this point. Now
  // we save all the pointers and schedule the last of the callbacks.
  m_device_manager.reset(device_manager.release());
  m_discovery_agent.reset(discovery_agent.release());
  m_plugin_adaptor.reset(plugin_adaptor.release());
  m_plugin_manager.reset(plugin_manager.release());
  m_port_broker.reset(port_broker.release());
  m_port_manager.reset(port_manager.release());
  m_rpc_server.reset(rpc_server.release());
  m_service_impl.reset(service_impl.release());
  m_universe_store.reset(universe_store.release());

  UpdatePidStore(pid_store.release());

  if (m_housekeeping_timeout != ola::thread::INVALID_TIMEOUT) {
    m_ss->RemoveTimeout(m_housekeeping_timeout);
  }

  m_housekeeping_timeout = m_ss->RegisterRepeatingTimeout(
      K_HOUSEKEEPING_TIMEOUT_MS,
      ola::NewCallback(this, &OlaServer::RunHousekeeping));

  // The plugin load procedure can take a while so we run it in the main loop.
  m_ss->Execute(
      ola::NewSingleCallback(m_plugin_manager.get(), &PluginManager::LoadAll));

  return true;
}

void OlaServer::ReloadPlugins() {
  m_ss->Execute(NewCallback(this, &OlaServer::ReloadPluginsInternal));
}

void OlaServer::ReloadPidStore() {
  // We load the PIDs in this thread, and then hand the RootPidStore over to
  // the main thread. This avoids doing disk I/O in the network thread.
  const RootPidStore* pid_store = RootPidStore::LoadFromDirectory(
      m_options.pid_data_dir);
  if (!pid_store) {
    return;
  }

  m_ss->Execute(NewCallback(this, &OlaServer::UpdatePidStore, pid_store));
}

void OlaServer::NewConnection(ola::io::ConnectedDescriptor *descriptor) {
  if (!descriptor) {
    return;
  }
  InternalNewConnection(m_rpc_server.get(), descriptor);
}

ola::network::GenericSocketAddress OlaServer::LocalRPCAddress() const {
  if (m_rpc_server.get()) {
    return m_rpc_server->ListenAddress();
  } else {
    return ola::network::GenericSocketAddress();
  }
}

void OlaServer::NewClient(RpcSession *session) {
  OlaClientService_Stub *stub = new OlaClientService_Stub(session->Channel());
  Client *client = new Client(stub, m_default_uid);
  session->SetData(static_cast<void*>(client));
  m_broker->AddClient(client);
}

void OlaServer::ClientRemoved(RpcSession *session) {
  auto_ptr<Client> client(reinterpret_cast<Client*>(session->GetData()));
  session->SetData(NULL);

  m_broker->RemoveClient(client.get());

  vector<Universe*> universe_list;
  m_universe_store->GetList(&universe_list);
  vector<Universe*>::iterator uni_iter;

  for (uni_iter = universe_list.begin();
       uni_iter != universe_list.end(); ++uni_iter) {
    (*uni_iter)->RemoveSourceClient(client.get());
    (*uni_iter)->RemoveSinkClient(client.get());
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
    (*iter)->CleanStaleSourceClients();
    if ((*iter)->IsActive() &&
        (*iter)->RDMDiscoveryInterval().Seconds() &&
        *now - (*iter)->LastRDMDiscovery() > (*iter)->RDMDiscoveryInterval()) {
      // run incremental discovery
      (*iter)->RunRDMDiscovery(NULL, false);
    }
  }
  return true;
}

#ifdef HAVE_LIBMICROHTTPD
bool OlaServer::StartHttpServer(ola::rpc::RpcServer *server,
                                const ola::network::Interface &iface) {
  if (!m_options.http_enable) {
    return true;
  }

  // create a pipe for the HTTP server to communicate with the main
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
    InternalNewConnection(server, pipe_descriptor.release());
    m_httpd.reset(httpd.release());
    return true;
  } else {
    pipe_descriptor->Close();
    return false;
  }
}
#endif  // HAVE_LIBMICROHTTPD

void OlaServer::StopPlugins() {
  if (m_plugin_manager.get()) {
    m_plugin_manager->UnloadAll();
  }
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
bool OlaServer::InternalNewConnection(
    ola::rpc::RpcServer *server,
    ola::io::ConnectedDescriptor *descriptor) {
  if (server) {
    return server->AddClient(descriptor);
  } else {
    delete descriptor;
    return false;
  }
}

void OlaServer::ReloadPluginsInternal() {
  OLA_INFO << "Reloading plugins";
  StopPlugins();
  m_plugin_manager->LoadAll();
}

void OlaServer::UpdatePidStore(const RootPidStore *pid_store) {
  OLA_INFO << "Updated PID definitions.";
#ifdef HAVE_LIBMICROHTTPD
  if (m_httpd.get()) {
    m_httpd->SetPidStore(pid_store);
  }
#endif  // HAVE_LIBMICROHTTPD

  m_pid_store.reset(pid_store);
  OLA_INFO << "PID store is at " << m_pid_store.get();
}
}  // namespace ola
