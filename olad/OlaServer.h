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
 * OlaServer.h
 * Interface for the ola server class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef OLAD_OLASERVER_H_
#define OLAD_OLASERVER_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ola/ExportMap.h"
#include "ola/base/Macro.h"
#include "ola/io/SelectServer.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/Socket.h"
#include "ola/network/TCPSocketFactory.h"
#include "ola/plugin_id.h"
#include "ola/rdm/PidStore.h"
#include "ola/rdm/UID.h"

namespace ola {


#ifdef HAVE_LIBMICROHTTPD
typedef class OladHTTPServer OladHTTPServer_t;
#else
typedef int OladHTTPServer_t;
#endif

/*
 * The main OlaServer class
 */
class OlaServer {
 public:
  struct Options {
    bool http_enable;  // run the http server
    bool http_localhost_only;  // restrict access to localhost only
    bool http_enable_quit;  // enable /quit
    unsigned int http_port;  // port to run the http server on
    std::string http_data_dir;  // directory that contains the static content
    std::string interface;
    std::string pid_data_dir;  // directory with the pid definitions.
  };

  OlaServer(class OlaClientServiceFactory *factory,
            const std::vector<class PluginLoader*> &plugin_loaders,
            class PreferencesFactory *preferences_factory,
            ola::io::SelectServer *ss,
            const Options &ola_options,
            ola::network::TCPAcceptingSocket *socket = NULL,
            ExportMap *export_map = NULL);
  ~OlaServer();

  bool Init();

  // Thread safe.
  void ReloadPlugins();
  void ReloadPidStore();

  void StopServer() { m_ss->Terminate(); }
  void NewConnection(ola::io::ConnectedDescriptor *descriptor);
  void NewTCPConnection(ola::network::TCPSocket *socket);
  void ChannelClosed(int read_descriptor);
  bool RunHousekeeping();

  static const unsigned int DEFAULT_HTTP_PORT = 9090;

 private :
  struct ClientEntry {
    ola::io::ConnectedDescriptor *client_descriptor;
    class OlaClientService *client_service;
  };

  typedef std::map<int, ClientEntry> ClientMap;

  class OlaClientServiceFactory *m_service_factory;
  std::vector<class PluginLoader*> m_plugin_loaders;
  ola::io::SelectServer *m_ss;
  ola::network::TCPSocketFactory m_tcp_socket_factory;
  ola::network::TCPAcceptingSocket *m_accepting_socket;

  std::auto_ptr<class ExportMap> m_our_export_map;
  class ExportMap *m_export_map;
  class PreferencesFactory *m_preferences_factory;
  class Preferences *m_universe_preferences;

  std::auto_ptr<class DeviceManager> m_device_manager;
  std::auto_ptr<class PluginManager> m_plugin_manager;
  std::auto_ptr<class PluginAdaptor> m_plugin_adaptor;
  std::auto_ptr<class UniverseStore> m_universe_store;
  std::auto_ptr<class PortManager> m_port_manager;
  std::auto_ptr<class OlaServerServiceImpl> m_service_impl;
  std::auto_ptr<class ClientBroker> m_broker;
  std::auto_ptr<class PortBroker> m_port_broker;
  std::auto_ptr<const ola::rdm::RootPidStore> m_pid_store;

  ola::thread::timeout_id m_housekeeping_timeout;
  ClientMap m_sd_to_service;
  std::auto_ptr<OladHTTPServer_t> m_httpd;
  std::auto_ptr<class DiscoveryAgentInterface> m_discovery_agent;
  const Options m_options;
  ola::rdm::UID m_default_uid;

#ifdef HAVE_LIBMICROHTTPD
  bool StartHttpServer(const ola::network::Interface &interface);
#endif
  void StopPlugins();
  void InternalNewConnection(ola::io::ConnectedDescriptor *descriptor);
  void CleanupConnection(ClientEntry client);
  void ReloadPluginsInternal();
  void UpdatePidStore(const ola::rdm::RootPidStore *pid_store);

  static const char UNIVERSE_PREFERENCES[];
  static const char K_CLIENT_VAR[];
  static const char K_UID_VAR[];
  static const char K_DISCOVERY_SERVICE_TYPE[];
  static const unsigned int K_HOUSEKEEPING_TIMEOUT_MS;

  DISALLOW_COPY_AND_ASSIGN(OlaServer);
};
}  // namespace ola
#endif  // OLAD_OLASERVER_H_
