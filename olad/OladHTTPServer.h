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
 * OladHTTPServer.h
 * Interface for the OLA HTTP class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_OLADHTTPSERVER_H_
#define OLAD_OLADHTTPSERVER_H_

#include <time.h>
#include <string>
#include <vector>
#include "ola/ExportMap.h"
#include "ola/client/OlaClient.h"
#include "ola/base/Macro.h"
#include "ola/http/HTTPServer.h"
#include "ola/http/OlaHTTPServer.h"
#include "ola/network/Interface.h"
#include "ola/rdm/PidStore.h"
#include "olad/RDMHTTPModule.h"

namespace ola {


/*
 * This is the main OLA HTTP Server
 */
class OladHTTPServer: public ola::http::OlaHTTPServer {
 public:
  struct OladHTTPServerOptions: public
      ola::http::HTTPServer::HTTPServerOptions {
   public:
    bool enable_quit;

    OladHTTPServerOptions()
        : ola::http::HTTPServer::HTTPServerOptions(),
          enable_quit(true) {
    }
  };

  OladHTTPServer(ExportMap *export_map,
                 const OladHTTPServerOptions &options,
                 ola::io::ConnectedDescriptor *client_socket,
                 class OlaServer *ola_server,
                 const ola::network::Interface &iface);
  virtual ~OladHTTPServer();

  bool Init();
  void SetPidStore(const ola::rdm::RootPidStore *pid_store);

  int JsonServerStats(const ola::http::HTTPRequest *request,
                      ola::http::HTTPResponse *response);
  int JsonUniversePluginList(const ola::http::HTTPRequest *request,
                             ola::http::HTTPResponse *response);
  int JsonPluginInfo(const ola::http::HTTPRequest *request,
                     ola::http::HTTPResponse *response);
  int SetPluginState(const ola::http::HTTPRequest *request,
                     ola::http::HTTPResponse *response);
  int JsonUniverseInfo(const ola::http::HTTPRequest *request,
                       ola::http::HTTPResponse *response);
  int JsonAvailablePorts(const ola::http::HTTPRequest *request,
                         ola::http::HTTPResponse *response);
  int CreateNewUniverse(const ola::http::HTTPRequest *request,
                        ola::http::HTTPResponse *response);
  int ModifyUniverse(const ola::http::HTTPRequest *request,
                     ola::http::HTTPResponse *response);

  int GetDmx(const ola::http::HTTPRequest *request,
             ola::http::HTTPResponse *response);
  int HandleSetDmx(const ola::http::HTTPRequest *request,
                   ola::http::HTTPResponse *response);
  int DisplayQuit(const ola::http::HTTPRequest *request,
                  ola::http::HTTPResponse *response);
  int ReloadPlugins(const ola::http::HTTPRequest *request,
                    ola::http::HTTPResponse *response);
  int ReloadPidStore(const ola::http::HTTPRequest *request,
                     ola::http::HTTPResponse *response);

  void HandlePluginList(ola::http::HTTPResponse *response,
                        const client::Result &result,
                        const std::vector<client::OlaPlugin> &plugins);

  void HandleUniverseList(ola::http::HTTPResponse *response,
                          ola::web::JsonObject *json,
                          const client::Result &result,
                          const std::vector<client::OlaUniverse> &universes);

  void HandlePartialPluginInfo(ola::http::HTTPResponse *response,
                               int plugin_id,
                               const client::Result &result,
                               const std::string &description);
  void HandlePluginInfo(ola::http::HTTPResponse *response,
                        std::string description,
                        const client::Result &result,
                        const ola::client::PluginState &state);

  void HandleUniverseInfo(ola::http::HTTPResponse *response,
                          const client::Result &result,
                          const client::OlaUniverse &universe);

  void HandlePortsForUniverse(ola::http::HTTPResponse *response,
                              ola::web::JsonObject *json,
                              unsigned int universe_id,
                              const client::Result &result,
                              const std::vector<client::OlaDevice> &devices);

  void HandleCandidatePorts(ola::http::HTTPResponse *response,
                            const client::Result &result,
                            const std::vector<client::OlaDevice> &devices);

  void CreateUniverseComplete(ola::http::HTTPResponse *response,
                              unsigned int universe_id,
                              bool included_name,
                              class ActionQueue *action_queue);

  void SendCreateUniverseResponse(ola::http::HTTPResponse *response,
                                  unsigned int universe_id,
                                  bool included_name,
                                  class ActionQueue *action_queue);

  void ModifyUniverseComplete(ola::http::HTTPResponse *response,
                              class ActionQueue *action_queue);
  void SendModifyUniverseResponse(ola::http::HTTPResponse *response,
                                  class ActionQueue *action_queue);

  /**
   * Serve a help redirect
   * @param response the response to use
   */
  inline static int ServeHelpRedirect(ola::http::HTTPResponse *response) {
    return ola::http::HTTPServer::ServeRedirect(response, HELP_REDIRECTION);
  }

  static int ServeUsage(ola::http::HTTPResponse *response,
                        const std::string &details);

  static const char HELP_PARAMETER[];

 private:
  class ola::io::ConnectedDescriptor *m_client_socket;
  ola::client::OlaClient m_client;
  class OlaServer *m_ola_server;
  bool m_enable_quit;
  ola::network::Interface m_interface;
  RDMHTTPModule m_rdm_module;
  time_t m_start_time_t;

  void HandleGetDmx(ola::http::HTTPResponse *response,
                    const client::Result &result,
                    const client::DMXMetadata &metadata,
                    const DmxBuffer &buffer);

  void HandleBoolResponse(ola::http::HTTPResponse *response,
                          const client::Result &result);

  void PortToJson(ola::web::JsonObject *object,
                  const client::OlaDevice &device,
                  const client::OlaPort &port,
                  bool is_output);

  void AddPatchActions(ActionQueue *action_queue,
                       const std::string port_id_string,
                       unsigned int universe,
                       client::PatchAction port_action);

  void AddPriorityActions(ActionQueue *action_queue,
                          const ola::http::HTTPRequest *request);

  typedef struct {
    unsigned int device_alias;
    unsigned int port;
    client::PortDirection direction;
    std::string string_id;
  } port_identifier;

  void DecodePortIds(const std::string &port_ids,
                     std::vector<port_identifier> *ports);

  void RegisterHandler(
      const std::string &path,
      int (OladHTTPServer::*method)(const ola::http::HTTPRequest*,
                                    ola::http::HTTPResponse*));

  static const char HELP_REDIRECTION[];
  static const char K_BACKEND_DISCONNECTED_ERROR[];
  static const unsigned int K_UNIVERSE_NAME_LIMIT = 100;
  static const char K_PRIORITY_VALUE_SUFFIX[];
  static const char K_PRIORITY_MODE_SUFFIX[];

  DISALLOW_COPY_AND_ASSIGN(OladHTTPServer);
};
}  // namespace ola
#endif  // OLAD_OLADHTTPSERVER_H_
