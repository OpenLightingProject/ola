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
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef OLAD_OLAHTTPSERVER_H_
#define OLAD_OLAHTTPSERVER_H_

#include <time.h>
#include <string>
#include <vector>
#include "ola/ExportMap.h"
#include "ola/OlaCallbackClient.h"
#include "ola/network/Interface.h"
#include "olad/HttpServer.h"
#include "olad/RDMHttpModule.h"

namespace ola {

using std::string;


/*
 * This is the main OLA HTTP Server
 */
class OlaHttpServer {
  public:
    OlaHttpServer(ExportMap *export_map,
                  ola::network::ConnectedDescriptor *client_socket,
                  class OlaServer *ola_server,
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
    int CreateNewUniverse(const HttpRequest *request, HttpResponse *response);
    int ModifyUniverse(const HttpRequest *request, HttpResponse *response);

    int DisplayIndex(const HttpRequest *request, HttpResponse *response);
    int GetDmx(const HttpRequest *request, HttpResponse *response);
    int HandleSetDmx(const HttpRequest *request, HttpResponse *response);
    int DisplayDebug(const HttpRequest *request, HttpResponse *response);
    int DisplayQuit(const HttpRequest *request, HttpResponse *response);
    int ReloadPlugins(const HttpRequest *request, HttpResponse *response);
    int DisplayHandlers(const HttpRequest *request, HttpResponse *response);

    void HandlePluginList(HttpResponse *response,
                          const vector<class OlaPlugin> &plugins,
                          const string &error);

    void HandleUniverseList(HttpResponse *response,
                            const vector<class OlaUniverse> &universes,
                            const string &error);

    void HandlePluginInfo(HttpResponse *response,
                          const string &description,
                          const string &error);

    void HandleUniverseInfo(HttpResponse *response,
                            class OlaUniverse &universe,
                            const string &error);

    void HandlePortsForUniverse(HttpResponse *response,
                                unsigned int universe_id,
                                const vector<class OlaDevice> &devices,
                                const string &error);

    void HandleCandidatePorts(HttpResponse *response,
                              const vector<class OlaDevice> &devices,
                              const string &error);

    void CreateUniverseComplete(HttpResponse *response,
                                unsigned int universe_id,
                                bool included_name,
                                class ActionQueue *action_queue);

    void SendCreateUniverseResponse(HttpResponse *response,
                                    unsigned int universe_id,
                                    bool included_name,
                                    class ActionQueue *action_queue);

    void ModifyUniverseComplete(HttpResponse *response,
                                class ActionQueue *action_queue);
    void SendModifyUniverseResponse(HttpResponse *response,
                                    class ActionQueue *action_queue);

  private:
    class HttpServer m_server;
    ExportMap *m_export_map;
    class ola::network::ConnectedDescriptor *m_client_socket;
    ola::OlaCallbackClient m_client;
    class OlaServer *m_ola_server;
    bool m_enable_quit;
    TimeStamp m_start_time;
    ola::network::Interface m_interface;
    RDMHttpModule m_rdm_module;
    time_t m_start_time_t;

    OlaHttpServer(const OlaHttpServer&);
    OlaHttpServer& operator=(const OlaHttpServer&);

    void HandleGetDmx(HttpResponse *response,
                      const DmxBuffer &buffer,
                      const string &error);

    void HandleBoolResponse(HttpResponse *response,
                            const string &error);

    void RegisterHandler(const string &path,
                         int (OlaHttpServer::*method)(const HttpRequest*,
                         HttpResponse*));
    void RegisterFile(const string &file, const string &content_type);

    void PortToJson(const class OlaDevice &device,
                    const class OlaPort &port,
                    stringstream *str,
                    bool is_output);

    void AddPatchActions(ActionQueue *action_queue,
                         const string port_id_string,
                         unsigned int universe,
                         PatchAction port_action);

    void AddPriorityActions(ActionQueue *action_queue,
                            const HttpRequest *request);

    typedef struct {
      unsigned int device_alias;
      unsigned int port;
      PortDirection direction;
      string string_id;
    } port_identifier;

    void DecodePortIds(const string &port_ids, vector<port_identifier> *ports);

    static const char K_DATA_DIR_VAR[];
    static const char K_UPTIME_VAR[];
    static const char K_BACKEND_DISCONNECTED_ERROR[];
    static const unsigned int K_UNIVERSE_NAME_LIMIT = 100;
    static const char K_PRIORITY_VALUE_SUFFIX[];
    static const char K_PRIORITY_MODE_SUFFIX[];
};
}  // ola
#endif  // OLAD_OLAHTTPSERVER_H_
