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
 * OlaHttpServer.h
 * Interface for the OLA HTTP class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef OLAD_OLAHTTPSERVER_H_
#define OLAD_OLAHTTPSERVER_H_

#include <time.h>
#include <string>
#include <vector>
#include "ola/Clock.h"
#include "ola/ExportMap.h"
#include "ola/OlaClient.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/SelectServer.h"
#include "olad/Device.h"
#include "olad/HttpServer.h"
#include "olad/PortManager.h"

namespace ola {

using std::string;
using ola::network::SelectServer;

class OlaHttpServer {
  public:
    OlaHttpServer(ExportMap *export_map,
                  SelectServer *ss,
                  ola::network::ConnectedSocket *client_socket,

                  class OlaServer *ola_server,
                  class UniverseStore *universe_store,
                  class PluginManager *plugin_manager,
                  class DeviceManager *device_manager,
                  PortManager *port_manager,
                  unsigned int port,
                  bool enable_quit,
                  const string &data_dir,
                  const ola::network::Interface &interface);
    ~OlaHttpServer();

    bool Init();
    bool Start() { return m_server.Start(); }
    void Stop() { return m_server.Stop(); }

    int JsonServerStats(const HttpRequest *request, HttpResponse *response);
    int JsonUniversePluginList(const HttpRequest *request,
                               HttpResponse *response);
    int JsonPluginInfo(const HttpRequest *request, HttpResponse *response);
    int JsonUniverseInfo(const HttpRequest *request, HttpResponse *response);
    int JsonAvailablePorts(const HttpRequest *request, HttpResponse *response);
    int JsonUIDs(const HttpRequest *request, HttpResponse *response);
    int CreateNewUniverse(const HttpRequest *request, HttpResponse *response);
    int ModifyUniverse(const HttpRequest *request, HttpResponse *response);

    int DisplayIndex(const HttpRequest *request, HttpResponse *response);
    int HandleSetDmx(const HttpRequest *request, HttpResponse *response);
    int DisplayDebug(const HttpRequest *request, HttpResponse *response);
    int DisplayQuit(const HttpRequest *request, HttpResponse *response);
    int ReloadPlugins(const HttpRequest *request, HttpResponse *response);
    int RunRDMDiscovery(const HttpRequest *request, HttpResponse *response);
    int DisplayHandlers(const HttpRequest *request, HttpResponse *response);

  private:
    OlaHttpServer(const OlaHttpServer&);
    OlaHttpServer& operator=(const OlaHttpServer&);

    void RegisterHandler(const string &path,
                         int (OlaHttpServer::*method)(const HttpRequest*,
                         HttpResponse*));
    void RegisterFile(const string &file, const string &content_type);

    template <class PortClass>
    void UpdatePortPriorites(const HttpRequest *request,
                             vector<PortClass*> *ports);

    void PortToJson(const Port *port, stringstream *str);
    bool UpdatePortsForUniverse(unsigned int universe_id,
                                const HttpRequest *request);

    template <class PortClass>
    void UpdatePortForUniverse(unsigned int universe_id,
                               PortClass *port,
                               const vector<string> &ids_to_add,
                               const vector<string> &ids_to_remove);

    class HttpServer m_server;
    ExportMap *m_export_map;
    SelectServer *m_ss;
    class ola::network::ConnectedSocket *m_client_socket;
    ola::OlaClient m_client;

    class OlaServer *m_ola_server;
    UniverseStore *m_universe_store;
    PluginManager *m_plugin_manager;
    DeviceManager *m_device_manager;
    PortManager *m_port_manager;
    bool m_enable_quit;
    TimeStamp m_start_time;
    ola::network::Interface m_interface;
    time_t m_start_time_t;

    static const char K_DATA_DIR_VAR[];
    static const char K_UPTIME_VAR[];
    static const unsigned int K_UNIVERSE_NAME_LIMIT = 100;
};
}  // ola
#endif  // OLAD_OLAHTTPSERVER_H_
