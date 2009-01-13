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
 * LlaHttpServer.h
 * Interface for the LLA HTTP class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_HTTP_SERVER_H
#define LLA_HTTP_SERVER_H

#include <string>
#include <google/template.h>
#include <lla/select_server/SelectServer.h>
#include <lla/ExportMap.h>
#include "HttpServer.h"

namespace lla {

using std::string;
using google::TemplateDictionary;
using lla::select_server::SelectServer;

class LlaHttpServer {
  public:
    LlaHttpServer(ExportMap *export_map,
                  SelectServer *ss,
                  class UniverseStore *universe_store,
                  class PluginLoader *plugin_loader,
                  class DeviceManager *device_manager,
                  unsigned int port,
                  bool enable_quit,
                  const string &data_dir);
    ~LlaHttpServer() {}
    bool Start() { return m_server.Start(); }
    void Stop() { return m_server.Stop(); }

    int DisplayIndex(const HttpRequest *request, HttpResponse *response);
    int DisplayMain(const HttpRequest *request, HttpResponse *response);
    int DisplayPlugins(const HttpRequest *request, HttpResponse *response);
    int DisplayPluginInfo(const HttpRequest *request, HttpResponse *response);
    int DisplayDevices(const HttpRequest *request, HttpResponse *response);
    int DisplayUniverses(const HttpRequest *request, HttpResponse *response);
    int DisplayConsole(const HttpRequest *request, HttpResponse *response);
    int HandleSetDmx(const HttpRequest *request, HttpResponse *response);
    int DisplayDebug(const HttpRequest *request, HttpResponse *response);
    int DisplayQuit(const HttpRequest *request, HttpResponse *response);
    int DisplayTemplateReload(const HttpRequest *request, HttpResponse *response);
    int DisplayHandlers(const HttpRequest *request, HttpResponse *response);

  private:
    LlaHttpServer(const LlaHttpServer&);
    LlaHttpServer& operator=(const LlaHttpServer&);

    void RegisterHandler(const string &path,
                         int (LlaHttpServer::*method)(const HttpRequest*,
                         HttpResponse*));
    void RegisterFile(const string &file, const string &content_type);
    void PopulateDeviceDict(const HttpRequest *request,
                            TemplateDictionary *dict,
                            AbstractDevice *device,
                            bool save_changes);
    string IntToString(int i);
    class HttpServer m_server;

    ExportMap *m_export_map;
    SelectServer *m_ss;
    UniverseStore *m_universe_store;
    PluginLoader *m_plugin_loader;
    DeviceManager *m_device_manager;
    bool m_enable_quit;

    static const string K_DATA_DIR_VAR;
    static const unsigned int K_UNIVERSE_NAME_LIMIT = 100;
    static const unsigned int K_CONSOLE_SLIDERS = 15;
};

} // lla
#endif
