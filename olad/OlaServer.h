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
#include <string>
#include <vector>

#include "ola/ExportMap.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "ola/plugin_id.h"
#include "ola/rdm/UID.h"

namespace ola {

#ifdef HAVE_LIBMICROHTTPD
typedef class OlaHttpServer OlaHttpServer_t;
#else
typedef int OlaHttpServer_t;
#endif

typedef struct {
  bool http_enable;  // run the http server
  bool http_localhost_only;  // restrict access to localhost only
  bool http_enable_quit;  // enable /quit
  unsigned int http_port;  // port to run the http server on
  std::string http_data_dir;  // directory that contains the static content
} ola_server_options;


/*
 * The main OlaServer class
 */
class OlaServer {
  public:
    OlaServer(class OlaServerServiceImplFactory *factory,
              const vector<class PluginLoader*> &plugin_loaders,
              class PreferencesFactory *preferences_factory,
              ola::network::SelectServer *ss,
              ola_server_options *ola_options,
              ola::network::AcceptingSocket *socket = NULL,
              ExportMap *export_map = NULL);
    ~OlaServer();
    bool Init();
    void ReloadPlugins();
    void StopServer() { m_ss->Terminate(); }
    void AcceptNewConnection(ola::network::AcceptingSocket *socket);
    bool NewConnection(ola::network::ConnectedSocket *socket);
    void SocketClosed(ola::network::ConnectedSocket *socket);
    bool RunHousekeeping();
    void CheckForReload();

    static const unsigned int DEFAULT_HTTP_PORT = 9090;

  private :
    OlaServer(const OlaServer&);
    OlaServer& operator=(const OlaServer&);

#ifdef HAVE_LIBMICROHTTPD
    bool StartHttpServer(const ola::network::Interface &interface);
#endif
    void StopPlugins();
    void CleanupConnection(class OlaServerServiceImpl *service);

    class OlaServerServiceImplFactory *m_service_factory;
    vector<class PluginLoader*> m_plugin_loaders;
    ola::network::SelectServer *m_ss;
    ola::network::AcceptingSocket *m_accepting_socket;

    class DeviceManager *m_device_manager;
    class PluginManager *m_plugin_manager;
    class PluginAdaptor *m_plugin_adaptor;
    class PreferencesFactory *m_preferences_factory;
    class Preferences *m_universe_preferences;
    class UniverseStore *m_universe_store;
    class ExportMap *m_export_map;
    class PortManager *m_port_manager;

    bool m_reload_plugins;
    bool m_init_run;
    bool m_free_export_map;
    ola::network::timeout_id m_housekeeping_timeout;
    std::map<int, class OlaServerServiceImpl*> m_sd_to_service;
    OlaHttpServer_t *m_httpd;
    ola_server_options m_options;
    class ClientBroker *m_broker;
    ola::rdm::UID m_default_uid;

    static const char UNIVERSE_PREFERENCES[];
    static const char K_CLIENT_VAR[];
    static const char K_UID_VAR[];
    static const unsigned int K_HOUSEKEEPING_TIMEOUT_MS;
};
}  // ola
#endif  // OLAD_OLASERVER_H_
