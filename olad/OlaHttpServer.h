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

#include <ctemplate/template.h>
#include <string>
#include <vector>
#include "ola/Clock.h"
#include "ola/ExportMap.h"
#include "ola/network/SelectServer.h"
#include "ola/network/InterfacePicker.h"
#include "olad/Device.h"
#include "olad/HttpServer.h"
#include "olad/PortManager.h"

namespace ola {

using std::string;
using ctemplate::TemplateDictionary;
using ola::network::SelectServer;

class OlaHttpServer {
  public:
    OlaHttpServer(ExportMap *export_map,
                  SelectServer *ss,
                  class OlaServer *ola_server,
                  class UniverseStore *universe_store,
                  class PluginManager *plugin_manager,
                  class DeviceManager *device_manager,
                  PortManager *port_manager,
                  unsigned int port,
                  bool enable_quit,
                  const string &data_dir,
                  const ola::network::Interface &interface);
    ~OlaHttpServer() {}
    bool Start() { return m_server.Start(); }
    void Stop() { return m_server.Stop(); }

    int DisplayIndex(const HttpRequest *request, HttpResponse *response);
    int DisplayMain(const HttpRequest *request, HttpResponse *response);
    int DisplayPlugins(const HttpRequest *request, HttpResponse *response);
    int DisplayPluginInfo(const HttpRequest *request, HttpResponse *response);
    int DisplayDevices(const HttpRequest *request, HttpResponse *response);
    int DisplayUniverses(const HttpRequest *request, HttpResponse *response);
    int DisplayRDM(const HttpRequest *request, HttpResponse *response);
    int DisplayConsole(const HttpRequest *request, HttpResponse *response);
    int HandleSetDmx(const HttpRequest *request, HttpResponse *response);
    int DisplayDebug(const HttpRequest *request, HttpResponse *response);
    int DisplayQuit(const HttpRequest *request, HttpResponse *response);
    int ReloadPlugins(const HttpRequest *request, HttpResponse *response);
    int DisplayTemplateReload(const HttpRequest *request,
                              HttpResponse *response);
    int DisplayHandlers(const HttpRequest *request, HttpResponse *response);

  private:
    OlaHttpServer(const OlaHttpServer&);
    OlaHttpServer& operator=(const OlaHttpServer&);

    void RegisterHandler(const string &path,
                         int (OlaHttpServer::*method)(const HttpRequest*,
                         HttpResponse*));
    void RegisterFile(const string &file, const string &content_type);

    void PopulateDeviceDict(const HttpRequest *request,
                            TemplateDictionary *dict,
                            const device_alias_pair &device_pair,
                            bool save_changes);

    template <class PortClass>
    void UpdatePortPatchings(const HttpRequest *request,
                             vector<PortClass*> *ports);

    template <class PortClass>
    void UpdatePortPriorites(const HttpRequest *request,
                             vector<PortClass*> *ports);

    template <class PortClass>
    void AddPortsToDict(TemplateDictionary *dict,
                        const vector<PortClass*> &ports,
                        unsigned int *offset);

    class HttpServer m_server;
    ExportMap *m_export_map;
    SelectServer *m_ss;
    class OlaServer *m_ola_server;
    UniverseStore *m_universe_store;
    PluginManager *m_plugin_manager;
    DeviceManager *m_device_manager;
    PortManager *m_port_manager;
    bool m_enable_quit;
    TimeStamp m_start_time;
    ola::network::Interface m_interface;

    static const char K_DATA_DIR_VAR[];
    static const char K_UPTIME_VAR[];
    static const unsigned int K_UNIVERSE_NAME_LIMIT = 100;
    static const unsigned int K_CONSOLE_SLIDERS = 15;
};
}  // ola
#endif  // OLAD_OLAHTTPSERVER_H_
