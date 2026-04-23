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
 * OlaServer.h
 * Interface for the ola server class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_OLASERVER_H_
#define OLAD_OLASERVER_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <ola/Constants.h>
#include <ola/ExportMap.h>
#include <ola/base/Macro.h>
#include <ola/io/SelectServer.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/Socket.h>
#include <ola/network/TCPSocketFactory.h>
#include <ola/plugin_id.h>
#include <ola/rdm/PidStore.h>
#include <ola/rdm/UID.h>
#include <ola/rpc/RpcSessionHandler.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ola {

namespace rpc {
class RpcSession;
class RpcServer;
}

#ifdef HAVE_LIBMICROHTTPD
typedef class OladHTTPServer OladHTTPServer_t;
#else
typedef int OladHTTPServer_t;
#endif  // HAVE_LIBMICROHTTPD

/**
 * @brief The main OlaServer class.
 */
class OlaServer : public ola::rpc::RpcSessionHandlerInterface {
 public:
  /**
   * @brief Options for the OlaServer.
   */
  struct Options {
    bool http_enable;  /** @brief Run the HTTP server */
    bool http_localhost_only;  /** @brief Restrict access to localhost only */
    bool http_enable_quit;  /** @brief Enable /quit URL */
    unsigned int http_port;  /** @brief Port to run the HTTP server on */
    /** @brief Directory that contains the static content */
    std::string http_data_dir;
    std::string network_interface;
    std::string pid_data_dir;  /** @brief Directory with the PID definitions */
  };

  /**
   * @brief Create a new instance of the OlaServer.
   * @param plugin_loaders A list of PluginLoaders to use to find plugins.
   * @param preferences_factory The factory to use when creating Preference
   *   objects.
   * @param ss The SelectServer.
   * @param ola_options The OlaServer options.
   * @param socket An optional TCPAcceptingSocket in the listen state to use
   *   for client RPC calls. Ownership is transferred.
   * @param export_map An optional ExportMap. If set to NULL a new ExportMap
   *   will be created.
   */
  OlaServer(const std::vector<class PluginLoader*> &plugin_loaders,
            class PreferencesFactory *preferences_factory,
            ola::io::SelectServer *ss,
            const Options &ola_options,
            ola::network::TCPAcceptingSocket *socket = NULL,
            ExportMap *export_map = NULL);

  /**
   * @brief Shutdown the server
   */
  ~OlaServer();

  /**
   * @brief Initialize the OlaServer.
   * @returns true if initialization succeeded, false if it failed.
   */
  bool Init();

  /**
   * @brief Reload all plugins.
   *
   * This method is thread safe.
   */
  void ReloadPlugins();

  /**
   * @brief Reload the pid store.
   *
   * This method is thread safe.
   */
  void ReloadPidStore();

  /**
   * @brief Stop the OLA Server.
   *
   * This terminates the underlying SelectServer.
   */
  void StopServer() { m_ss->Terminate(); }

  /**
   * @brief Add a new ConnectedDescriptor to this Server.
   * @param descriptor the new ConnectedDescriptor, ownership is transferred.
   */
  void NewConnection(ola::io::ConnectedDescriptor *descriptor);

  /**
   * @brief Return the socket address the RPC server is listening on.
   * @returns A socket address, which is empty if the server hasn't been
   *   initialized.
   */
  ola::network::GenericSocketAddress LocalRPCAddress() const;

  // Called by the RpcServer when clients connect or disconnect.
  void NewClient(ola::rpc::RpcSession *session);
  void ClientRemoved(ola::rpc::RpcSession *session);

  /**
   * @brief Get the instance name
   * @return a string which is the instance name
   */
  const std::string InstanceName() {
    return m_instance_name;
  }

  /**
   * @brief Get the preferences factory
   * @return a pointer to the preferences factory
   */
  const PreferencesFactory* GetPreferencesFactory() {
    return m_preferences_factory;
  }

  static const unsigned int DEFAULT_HTTP_PORT = 9090;

  static const unsigned int DEFAULT_RPC_PORT = OLA_DEFAULT_PORT;

 private :
  struct ClientEntry {
    ola::io::ConnectedDescriptor *client_descriptor;
    class OlaClientService *client_service;
  };

  typedef std::map<ola::io::DescriptorHandle, ClientEntry> ClientMap;

  // These are all passed to the constructor.
  const Options m_options;
  std::vector<class PluginLoader*> m_plugin_loaders;
  class PreferencesFactory *m_preferences_factory;
  ola::io::SelectServer *m_ss;
  ola::network::TCPAcceptingSocket *m_accepting_socket;
  class ExportMap *m_export_map;

  std::unique_ptr<class ExportMap> m_our_export_map;
  ola::rdm::UID m_default_uid;

  // These are all populated in Init.
  std::unique_ptr<class DeviceManager> m_device_manager;
  std::unique_ptr<class PluginManager> m_plugin_manager;
  std::unique_ptr<class PluginAdaptor> m_plugin_adaptor;
  std::unique_ptr<class UniverseStore> m_universe_store;
  std::unique_ptr<class PortManager> m_port_manager;
  std::unique_ptr<class OlaServerServiceImpl> m_service_impl;
  std::unique_ptr<class ClientBroker> m_broker;
  std::unique_ptr<class PortBroker> m_port_broker;
  std::unique_ptr<const ola::rdm::RootPidStore> m_pid_store;
  std::unique_ptr<class DiscoveryAgentInterface> m_discovery_agent;
  std::unique_ptr<ola::rpc::RpcServer> m_rpc_server;
  class Preferences *m_server_preferences;
  class Preferences *m_universe_preferences;
  std::string m_instance_name;

  ola::thread::timeout_id m_housekeeping_timeout;
  std::unique_ptr<OladHTTPServer_t> m_httpd;

  bool RunHousekeeping();

#ifdef HAVE_LIBMICROHTTPD
  bool StartHttpServer(ola::rpc::RpcServer *server,
                       const ola::network::Interface &iface);
#endif  // HAVE_LIBMICROHTTPD
  /**
   * @brief Stop and unload all the plugins
   */
  void StopPlugins();
  bool InternalNewConnection(ola::rpc::RpcServer *server,
                             ola::io::ConnectedDescriptor *descriptor);
  void ReloadPluginsInternal();
  /**
   * @brief Update the Pid store with the new values.
   */
  void UpdatePidStore(const ola::rdm::RootPidStore *pid_store);

  static const char INSTANCE_NAME_KEY[];
  static const char K_INSTANCE_NAME_VAR[];
  static const char K_DISCOVERY_SERVICE_TYPE[];
  static const char K_UID_VAR[];
  static const char SERVER_PREFERENCES[];
  static const char UNIVERSE_PREFERENCES[];
  static const unsigned int K_HOUSEKEEPING_TIMEOUT_MS;

  DISALLOW_COPY_AND_ASSIGN(OlaServer);
};
}  // namespace ola
#endif  // OLAD_OLASERVER_H_
