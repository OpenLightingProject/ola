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
 * OladHTTPServer.h
 * Interface for the OLA HTTP class
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef OLAD_OLADHTTPSERVER_H_
#define OLAD_OLADHTTPSERVER_H_

#include <time.h>
#include <string>
#include <vector>
#include "ola/ExportMap.h"
#include "ola/OlaCallbackClient.h"
#include "ola/http/HTTPServer.h"
#include "ola/http/OlaHTTPServer.h"
#include "ola/network/Interface.h"
#include "olad/RDMHTTPModule.h"

namespace ola {

using std::string;
using ola::http::HTTPServer;


/*
 * This is the main OLA HTTP Server
 */
class OladHTTPServer: public ola::http::OlaHTTPServer {
  public:
    struct OladHTTPServerOptions: public HTTPServer::HTTPServerOptions {
      public:
        bool enable_quit;

      OladHTTPServerOptions()
          : HTTPServer::HTTPServerOptions(),
            enable_quit(true) {
      }
    };

    OladHTTPServer(ExportMap *export_map,
                   const OladHTTPServerOptions &options,
                   ola::io::ConnectedDescriptor *client_socket,
                   class OlaServer *ola_server,
                   const ola::network::Interface &interface);
    virtual ~OladHTTPServer();

    bool Init();

    int JsonServerStats(const HTTPRequest *request, HTTPResponse *response);
    int JsonUniversePluginList(const HTTPRequest *request,
                               HTTPResponse *response);
    int JsonPluginInfo(const HTTPRequest *request, HTTPResponse *response);
    int JsonUniverseInfo(const HTTPRequest *request, HTTPResponse *response);
    int JsonAvailablePorts(const HTTPRequest *request, HTTPResponse *response);
    int CreateNewUniverse(const HTTPRequest *request, HTTPResponse *response);
    int ModifyUniverse(const HTTPRequest *request, HTTPResponse *response);

    int GetDmx(const HTTPRequest *request, HTTPResponse *response);
    int HandleSetDmx(const HTTPRequest *request, HTTPResponse *response);
    int DisplayQuit(const HTTPRequest *request, HTTPResponse *response);
    int ReloadPlugins(const HTTPRequest *request, HTTPResponse *response);

    void HandlePluginList(HTTPResponse *response,
                          const vector<class OlaPlugin> &plugins,
                          const string &error);

    void HandleUniverseList(HTTPResponse *response,
                            ola::web::JsonObject *json,
                            const vector<class OlaUniverse> &universes,
                            const string &error);

    void HandlePartialPluginInfo(HTTPResponse *response,
                                 int plugin_id,
                                 const string &description,
                                 const string &error);
    void HandlePluginInfo(HTTPResponse *response,
                          string description,
                          const OlaCallbackClient::PluginState &state,
                          const string &error);

    void HandleUniverseInfo(HTTPResponse *response,
                            class OlaUniverse &universe,
                            const string &error);

    void HandlePortsForUniverse(HTTPResponse *response,
                                ola::web::JsonObject *json,
                                unsigned int universe_id,
                                const vector<class OlaDevice> &devices,
                                const string &error);

    void HandleCandidatePorts(HTTPResponse *response,
                              const vector<class OlaDevice> &devices,
                              const string &error);

    void CreateUniverseComplete(HTTPResponse *response,
                                unsigned int universe_id,
                                bool included_name,
                                class ActionQueue *action_queue);

    void SendCreateUniverseResponse(HTTPResponse *response,
                                    unsigned int universe_id,
                                    bool included_name,
                                    class ActionQueue *action_queue);

    void ModifyUniverseComplete(HTTPResponse *response,
                                class ActionQueue *action_queue);
    void SendModifyUniverseResponse(HTTPResponse *response,
                                    class ActionQueue *action_queue);

    /*
     * Serve a help redirect
     * @param response the response to use
     */
    inline static int ServeHelpRedirect(HTTPResponse *response) {
      return HTTPServer::ServeRedirect(response, HELP_REDIRECTION);
    }

    static int ServeUsage(HTTPResponse *response, const string &details);

    static const char HELP_PARAMETER[];

  private:
    class ola::io::ConnectedDescriptor *m_client_socket;
    ola::OlaCallbackClient m_client;
    class OlaServer *m_ola_server;
    bool m_enable_quit;
    ola::network::Interface m_interface;
    RDMHTTPModule m_rdm_module;
    time_t m_start_time_t;

    OladHTTPServer(const OladHTTPServer&);
    OladHTTPServer& operator=(const OladHTTPServer&);

    void HandleGetDmx(HTTPResponse *response,
                      const DmxBuffer &buffer,
                      const string &error);

    void HandleBoolResponse(HTTPResponse *response, const string &error);

    void PortToJson(ola::web::JsonObject *object,
                    const class OlaDevice &device,
                    const class OlaPort &port,
                    bool is_output);

    void AddPatchActions(ActionQueue *action_queue,
                         const string port_id_string,
                         unsigned int universe,
                         PatchAction port_action);

    void AddPriorityActions(ActionQueue *action_queue,
                            const HTTPRequest *request);

    typedef struct {
      unsigned int device_alias;
      unsigned int port;
      PortDirection direction;
      string string_id;
    } port_identifier;

    void DecodePortIds(const string &port_ids, vector<port_identifier> *ports);

    void RegisterHandler(
        const string &path,
        int (OladHTTPServer::*method)(const HTTPRequest*, HTTPResponse*));

    static const char HELP_REDIRECTION[];
    static const char K_BACKEND_DISCONNECTED_ERROR[];
    static const unsigned int K_UNIVERSE_NAME_LIMIT = 100;
    static const char K_PRIORITY_VALUE_SUFFIX[];
    static const char K_PRIORITY_MODE_SUFFIX[];
};
}  // ola
#endif  // OLAD_OLADHTTPSERVER_H_
