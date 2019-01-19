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
 * OladHTTPServer.cpp
 * Ola HTTP class
 * Copyright (C) 2005 Simon Newton
 */

#include <sys/time.h>
#include <iostream>
#include <string>
#include <vector>

#include "ola/ActionQueue.h"
#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/base/Version.h"
#include "ola/dmx/SourcePriorities.h"
#include "ola/network/NetworkUtils.h"
#include "ola/web/Json.h"
#include "olad/DmxSource.h"
#include "olad/HttpServerActions.h"
#include "olad/OladHTTPServer.h"
#include "olad/OlaServer.h"
#include "olad/Preferences.h"

namespace ola {

using ola::client::OlaDevice;
using ola::client::OlaInputPort;
using ola::client::OlaOutputPort;
using ola::client::OlaPlugin;
using ola::client::OlaPort;
using ola::client::OlaUniverse;
using ola::http::HTTPRequest;
using ola::http::HTTPResponse;
using ola::http::HTTPServer;
using ola::io::ConnectedDescriptor;
using ola::web::JsonArray;
using ola::web::JsonObject;
using std::cout;
using std::endl;
using std::ostringstream;
using std::string;
using std::vector;

const char OladHTTPServer::HELP_PARAMETER[] = "help";
const char OladHTTPServer::HELP_REDIRECTION[] = "?help=1";
const char OladHTTPServer::K_BACKEND_DISCONNECTED_ERROR[] =
    "Failed to send request, client isn't connected";
const char OladHTTPServer::K_PRIORITY_VALUE_SUFFIX[] = "_priority_value";
const char OladHTTPServer::K_PRIORITY_MODE_SUFFIX[] = "_priority_mode";

/**
 * @brief Create a new OLA HTTP server
 * @param export_map the ExportMap to display when /debug is called
 * @param options the OladHTTPServerOptions for the OLA HTTP server
 * @param client_socket A ConnectedDescriptor which is used to communicate with
 *   the server.
 * @param ola_server the OlaServer to use
 * @param iface the network interface to bind to
 */
OladHTTPServer::OladHTTPServer(ExportMap *export_map,
                               const OladHTTPServerOptions &options,
                               ConnectedDescriptor *client_socket,
                               OlaServer *ola_server,
                               const ola::network::Interface &iface)
    : OlaHTTPServer(options, export_map),
      m_client_socket(client_socket),
      m_client(client_socket),
      m_ola_server(ola_server),
      m_enable_quit(options.enable_quit),
      m_interface(iface),
      m_rdm_module(&m_server, &m_client) {
  // The main handlers
  RegisterHandler("/quit", &OladHTTPServer::DisplayQuit);
  RegisterHandler("/reload", &OladHTTPServer::ReloadPlugins);
  RegisterHandler("/reload_pids", &OladHTTPServer::ReloadPidStore);
  RegisterHandler("/new_universe", &OladHTTPServer::CreateNewUniverse);
  RegisterHandler("/modify_universe", &OladHTTPServer::ModifyUniverse);
  RegisterHandler("/set_plugin_state", &OladHTTPServer::SetPluginState);
  RegisterHandler("/set_dmx", &OladHTTPServer::HandleSetDmx);
  RegisterHandler("/get_dmx", &OladHTTPServer::GetDmx);

  // json endpoints for the new UI
  RegisterHandler("/json/server_stats", &OladHTTPServer::JsonServerStats);
  RegisterHandler("/json/universe_plugin_list",
                  &OladHTTPServer::JsonUniversePluginList);
  RegisterHandler("/json/plugin_info", &OladHTTPServer::JsonPluginInfo);
  RegisterHandler("/json/get_ports", &OladHTTPServer::JsonAvailablePorts);
  RegisterHandler("/json/universe_info", &OladHTTPServer::JsonUniverseInfo);

  // these are the static files for the old UI
  m_server.RegisterFile("/blank.gif", HTTPServer::CONTENT_TYPE_GIF);
  m_server.RegisterFile("/button-bg.png", HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile("/custombutton.css", HTTPServer::CONTENT_TYPE_CSS);
  m_server.RegisterFile("/editortoolbar.png", HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile("/expander.png", HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile("/handle.vertical.png", HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile("/loader.gif", HTTPServer::CONTENT_TYPE_GIF);
  m_server.RegisterFile("/loader-mini.gif", HTTPServer::CONTENT_TYPE_GIF);
  m_server.RegisterFile("/logo.png", HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile("/logo-mini.png", HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile("/mobile.html", HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile("/mobile.js", HTTPServer::CONTENT_TYPE_JS);
  m_server.RegisterFile("/ola.html", HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile("/ola.js", HTTPServer::CONTENT_TYPE_JS);
  m_server.RegisterFile("/tick.gif", HTTPServer::CONTENT_TYPE_GIF);
  m_server.RegisterFile("/toolbar-bg.png", HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile("/toolbar.css", HTTPServer::CONTENT_TYPE_CSS);
  m_server.RegisterFile("/toolbar_sprites.png", HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile("/vertical.gif", HTTPServer::CONTENT_TYPE_GIF);
  m_server.RegisterFile("/warning.png", HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile("/", "landing.html", HTTPServer::CONTENT_TYPE_HTML);

  // these are the static files for the new UI
  m_server.RegisterFile(
      "/new/",
      "/new/index.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/overview.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/plugins.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/plugin-info.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/universe-overview.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/universe-add.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/universe-header.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/universe-keypad.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/universe-patch.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/universe-settings.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/universe-faders.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/universes.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/views/universe-rdm.html",
      HTTPServer::CONTENT_TYPE_HTML);
  m_server.RegisterFile(
      "/new/js/app.min.js",
      HTTPServer::CONTENT_TYPE_JS);
  m_server.RegisterFile(
      "/new/js/app.min.js.map",
      HTTPServer::CONTENT_TYPE_OCT);
  m_server.RegisterFile(
      "/new/libs/jquery/js/jquery.min.js",
      HTTPServer::CONTENT_TYPE_JS);
  m_server.RegisterFile(
      "/new/libs/angular-route/js/angular-route.min.js",
      HTTPServer::CONTENT_TYPE_JS);
  m_server.RegisterFile(
      "/new/libs/angular/js/angular.min.js",
      HTTPServer::CONTENT_TYPE_JS);
  m_server.RegisterFile(
      "/new/libs/marked/js/marked.min.js",
      HTTPServer::CONTENT_TYPE_JS);
  m_server.RegisterFile(
      "/new/libs/angular-marked/js/angular-marked.min.js",
      HTTPServer::CONTENT_TYPE_JS);
  m_server.RegisterFile(
      "/new/libs/bootstrap/js/bootstrap.min.js",
      HTTPServer::CONTENT_TYPE_JS);
  m_server.RegisterFile(
      "/new/libs/bootstrap/fonts/glyphicons-halflings-regular.woff",
      HTTPServer::CONTENT_TYPE_OCT);
  m_server.RegisterFile(
      "/new/libs/bootstrap/fonts/glyphicons-halflings-regular.svg",
      HTTPServer::CONTENT_TYPE_OCT);
  m_server.RegisterFile(
      "/new/libs/bootstrap/fonts/glyphicons-halflings-regular.ttf",
      HTTPServer::CONTENT_TYPE_OCT);
  m_server.RegisterFile(
      "/new/libs/bootstrap/fonts/glyphicons-halflings-regular.eot",
      HTTPServer::CONTENT_TYPE_OCT);
  m_server.RegisterFile(
      "/new/libs/bootstrap/fonts/glyphicons-halflings-regular.woff2",
      HTTPServer::CONTENT_TYPE_OCT);
  m_server.RegisterFile(
      "/new/css/style.min.css",
      HTTPServer::CONTENT_TYPE_CSS);
  m_server.RegisterFile(
      "/new/libs/bootstrap/css/bootstrap.min.css",
      HTTPServer::CONTENT_TYPE_CSS);
  m_server.RegisterFile(
      "/new/img/logo.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/light_bulb.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/light_bulb_off.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/logo-mini.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/logo-mini@2x.png",
      HTTPServer::CONTENT_TYPE_PNG);

  // Favicons for the new UI
  m_server.RegisterFile(
      "/new/img/favicons/android-chrome-144x144.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/android-chrome-192x192.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/android-chrome-36x36.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/android-chrome-48x48.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/android-chrome-72x72.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/android-chrome-96x96.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-icon-114x114.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-icon-120x120.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-icon-144x144.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-icon-152x152.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-icon-180x180.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-icon-57x57.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-icon-60x60.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-icon-72x72.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-icon-76x76.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-icon-precomposed.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-icon.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-startup-image-1182x2208.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-startup-image-1242x2148.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-startup-image-1496x2048.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-startup-image-1536x2008.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-startup-image-320x460.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-startup-image-640x1096.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-startup-image-640x920.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-startup-image-748x1024.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-startup-image-750x1294.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/apple-touch-startup-image-768x1004.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/browserconfig.xml",
      HTTPServer::CONTENT_TYPE_XML);
  m_server.RegisterFile(
      "/new/img/favicons/coast-228x228.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/favicon-16x16.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/favicon-230x230.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/favicon-32x32.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/favicon-96x96.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/favicon.ico",
      HTTPServer::CONTENT_TYPE_ICO);
  m_server.RegisterFile(
      "/new/img/favicons/firefox_app_128x128.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/firefox_app_512x512.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/firefox_app_60x60.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/manifest.json",
      HTTPServer::CONTENT_TYPE_JSON);
  m_server.RegisterFile(
      "/new/img/favicons/manifest.webapp",
      HTTPServer::CONTENT_TYPE_PLAIN);
  m_server.RegisterFile(
      "/new/img/favicons/mstile-144x144.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/mstile-150x150.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/mstile-310x150.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/mstile-310x310.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/mstile-70x70.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/open-graph.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/twitter.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/yandex-browser-50x50.png",
      HTTPServer::CONTENT_TYPE_PNG);
  m_server.RegisterFile(
      "/new/img/favicons/yandex-browser-manifest.json",
      HTTPServer::CONTENT_TYPE_JSON);

  m_start_time_t = time(NULL);
}


/*
 * @brief Teardown
 */
OladHTTPServer::~OladHTTPServer() {
  if (m_client_socket) {
    m_server.SelectServer()->RemoveReadDescriptor(m_client_socket);
  }
  m_client.Stop();
  if (m_client_socket) {
    delete m_client_socket;
  }
}


/**
 * @brief Setup the OLA HTTP server
 * @return true if this worked, false otherwise.
 */
bool OladHTTPServer::Init() {
  if (!OlaHTTPServer::Init()) {
    return false;
  }

  if (!m_client.Setup()) {
    return false;
  }
  /*
  Setup disconnect notifications.
  m_socket->SetOnClose(
    ola::NewSingleCallback(this, &SimpleClient::SocketClosed));
  */
  m_server.SelectServer()->AddReadDescriptor(m_client_socket);
  return true;
}


/**
 * @brief Can be called while the HTTP server is running
 */
void OladHTTPServer::SetPidStore(const ola::rdm::RootPidStore *pid_store) {
  m_rdm_module.SetPidStore(pid_store);
}


/**
 * @brief Print the server stats JSON
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::JsonServerStats(const HTTPRequest*,
                                    HTTPResponse *response) {
  char start_time_str[50];
#ifdef _WIN32
  strftime(start_time_str, sizeof(start_time_str), "%c",
      localtime(&m_start_time_t));
#else
  struct tm start_time;
  localtime_r(&m_start_time_t, &start_time);
  strftime(start_time_str, sizeof(start_time_str), "%c", &start_time);
#endif  // _WIN32

  JsonObject json;
#ifdef OLA_SCREENSHOT_MODE
  json.Add("hostname", "***");
  json.Add("instance_name", "***");
  json.Add("config_dir", "***");
  json.Add("ip", "***.***.***.***");
  json.Add("broadcast", "***.***.***.***");
  json.Add("subnet", "***.***.***.***");
  json.Add("hw_address", "**:**:**:**:**:**");
#else
  json.Add("hostname", ola::network::FQDN());
  json.Add("instance_name", m_ola_server->InstanceName());
  json.Add("config_dir",
           m_ola_server->GetPreferencesFactory()->ConfigLocation());
  json.Add("ip", m_interface.ip_address.ToString());
  json.Add("broadcast", m_interface.bcast_address.ToString());
  json.Add("subnet", m_interface.subnet_mask.ToString());
  json.Add("hw_address", m_interface.hw_address.ToString());
#endif  // OLA_SCREENSHOT_MODE
  json.Add("version", ola::base::Version::GetVersion());
  json.Add("up_since", start_time_str);
  json.Add("quit_enabled", m_enable_quit);

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  int r = response->SendJson(json);
  delete response;
  return r;
}


/**
 * @brief Print the list of universes / plugins as a json string
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::JsonUniversePluginList(const HTTPRequest*,
                                           HTTPResponse *response) {
  m_client.FetchPluginList(
      NewSingleCallback(this,
                        &OladHTTPServer::HandlePluginList,
                        response));
  return MHD_YES;
}


/**
 * @brief Print the plugin info as a JSON string
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::JsonPluginInfo(const HTTPRequest *request,
                                   HTTPResponse *response) {
  if (request->CheckParameterExists(HELP_PARAMETER)) {
    return ServeUsage(response, "?id=[plugin]");
  }
  string val = request->GetParameter("id");
  int plugin_id;
  if (!StringToInt(val, &plugin_id)) {
    return ServeHelpRedirect(response);
  }

  m_client.FetchPluginDescription(
      (ola_plugin_id) plugin_id,
      NewSingleCallback(this,
                        &OladHTTPServer::HandlePartialPluginInfo,
                        response, plugin_id));
  return MHD_YES;
}


/**
 * @brief Return information about a universe
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::JsonUniverseInfo(OLA_UNUSED const HTTPRequest *request,
                                     HTTPResponse *response) {
  if (request->CheckParameterExists(HELP_PARAMETER)) {
    return ServeUsage(response, "?id=[universe]");
  }
  string uni_id = request->GetParameter("id");
  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id)) {
    return ServeHelpRedirect(response);
  }

  m_client.FetchUniverseInfo(
      universe_id,
      NewSingleCallback(this,
                        &OladHTTPServer::HandleUniverseInfo,
                        response));

  return MHD_YES;
}


/**
 * @brief Return a list of unbound ports
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::JsonAvailablePorts(const HTTPRequest *request,
                                       HTTPResponse *response) {
  if (request->CheckParameterExists(HELP_PARAMETER)) {
    return ServeUsage(response, "? or ?id=[universe]");
  }
  string uni_id = request->GetParameter("id");

  if (uni_id.empty()) {
    // get all available ports
    m_client.FetchCandidatePorts(
        NewSingleCallback(this,
                          &OladHTTPServer::HandleCandidatePorts,
                          response));
  } else {
    unsigned int universe_id;
    if (!StringToInt(uni_id, &universe_id)) {
      return ServeHelpRedirect(response);
    }

    m_client.FetchCandidatePorts(
        universe_id,
        NewSingleCallback(this,
                          &OladHTTPServer::HandleCandidatePorts,
                          response));
  }
  return MHD_YES;
}


/**
 * @brief Create a new universe by binding one or more ports.
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::CreateNewUniverse(const HTTPRequest *request,
                                      HTTPResponse *response) {
  if (request->CheckParameterExists(HELP_PARAMETER)) {
    return ServeUsage(response,
                      "POST id=[universe], name=[name], add_ports=[a comma "
                      "separated list of port ids]");
  }
  string uni_id = request->GetPostParameter("id");
  string name = request->GetPostParameter("name");

  if (name.size() > K_UNIVERSE_NAME_LIMIT) {
    name = name.substr(K_UNIVERSE_NAME_LIMIT);
  }

  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id)) {
    return ServeHelpRedirect(response);
  }

  ActionQueue *action_queue = new ActionQueue(
      NewSingleCallback(this,
                        &OladHTTPServer::CreateUniverseComplete,
                        response,
                        universe_id,
                        !name.empty()));

  // add patch actions here
  string add_port_ids = request->GetPostParameter("add_ports");
  AddPatchActions(action_queue, add_port_ids, universe_id, ola::client::PATCH);

  if (!name.empty()) {
    action_queue->AddAction(
        new SetNameAction(&m_client, universe_id, name, false));
  }

  action_queue->NextAction();
  return MHD_YES;
}


/**
 * @brief Modify an existing universe.
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::ModifyUniverse(const HTTPRequest *request,
                                   HTTPResponse *response) {
  if (request->CheckParameterExists(HELP_PARAMETER)) {
    return ServeUsage(response,
                      "POST id=[universe], name=[name], merge_mode=[HTP|LTP], "
                      "add_ports=[a comma separated list of port ids], "
                      "remove_ports=[a comma separated list of port ids]");
  }

  string uni_id = request->GetPostParameter("id");
  string name = request->GetPostParameter("name");
  string merge_mode = request->GetPostParameter("merge_mode");

  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id)) {
    return ServeHelpRedirect(response);
  }

  if (name.empty()) {
    return m_server.ServeError(response, "No name supplied");
  }

  if (name.size() > K_UNIVERSE_NAME_LIMIT) {
    name = name.substr(K_UNIVERSE_NAME_LIMIT);
  }

  ActionQueue *action_queue = new ActionQueue(
      NewSingleCallback(this,
                        &OladHTTPServer::ModifyUniverseComplete,
                        response));

  action_queue->AddAction(
      new SetNameAction(&m_client, universe_id, name, true));

  if (merge_mode == "LTP" || merge_mode == "HTP") {
    OlaUniverse::merge_mode mode = (
        merge_mode == "LTP" ? OlaUniverse::MERGE_LTP : OlaUniverse::MERGE_HTP);
    action_queue->AddAction(
        new SetMergeModeAction(&m_client, universe_id, mode));
  }

  string remove_port_ids = request->GetPostParameter("remove_ports");
  AddPatchActions(action_queue, remove_port_ids, universe_id, client::UNPATCH);

  string add_port_ids = request->GetPostParameter("add_ports");
  AddPatchActions(action_queue, add_port_ids, universe_id, client::PATCH);

  AddPriorityActions(action_queue, request);

  action_queue->NextAction();
  return MHD_YES;
}

/**
 * @brief Set plugin state.
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::SetPluginState(const HTTPRequest *request,
                                   HTTPResponse *response) {
  if (request->CheckParameterExists(HELP_PARAMETER)) {
    return ServeUsage(response,
                      "POST state=[enable|disable], "
                      "plugin_id=[a plugin id]");
  }

  string state_string = request->GetPostParameter("state");

  bool state;
  if (!StringToBoolTolerant(state_string, &state)) {
    OLA_INFO << "Invalid state " << state_string;
    return ServeHelpRedirect(response);
  }

  string plugin_id_string = request->GetPostParameter("plugin_id");
  unsigned int plugin_id;
  if (!StringToInt(plugin_id_string, &plugin_id)) {
    OLA_INFO << "Invalid plugin id " << plugin_id_string;
    return ServeHelpRedirect(response);
  }

  m_client.SetPluginState(
      (ola_plugin_id) plugin_id,
      state,
      NewSingleCallback(this, &OladHTTPServer::HandleBoolResponse, response));

  return MHD_YES;
}


/**
 * @brief Handle the get DMX command
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::GetDmx(const HTTPRequest *request,
                           HTTPResponse *response) {
  if (request->CheckParameterExists(HELP_PARAMETER)) {
    return ServeUsage(response, "?u=[universe]");
  }
  string uni_id = request->GetParameter("u");
  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id)) {
    return ServeHelpRedirect(response);
  }

  m_client.FetchDMX(
      universe_id,
      NewSingleCallback(this, &OladHTTPServer::HandleGetDmx, response));
  return MHD_YES;
}


/**
 * @brief Handle the set DMX command
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::HandleSetDmx(const HTTPRequest *request,
                                 HTTPResponse *response) {
  if (request->CheckParameterExists(HELP_PARAMETER)) {
    return ServeUsage(response,
        "POST u=[universe], d=[DMX data (a comma separated list of values)]");
  }
  string dmx_data_str = request->GetPostParameter("d");
  string uni_id = request->GetPostParameter("u");
  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id)) {
    return ServeHelpRedirect(response);
  }

  DmxBuffer buffer;
  buffer.SetFromString(dmx_data_str);
  if (!buffer.Size()) {
    return m_server.ServeError(response, "Invalid DMX string");
  }

  ola::client::SendDMXArgs args(
      NewSingleCallback(this, &OladHTTPServer::HandleBoolResponse, response));
  m_client.SendDMX(universe_id, buffer, args);
  return MHD_YES;
}


/**
 * @brief Cause the server to shutdown
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::DisplayQuit(OLA_UNUSED const HTTPRequest *request,
                                HTTPResponse *response) {
  if (m_enable_quit) {
    response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
    response->Append("ok");
    m_ola_server->StopServer();
  } else {
    response->SetStatus(403);
    response->SetContentType(HTTPServer::CONTENT_TYPE_HTML);
    response->Append("<b>403 Unauthorized</b>");
  }
  response->SetNoCache();
  int r = response->Send();
  delete response;
  return r;
}


/**
 * @brief Reload all plugins
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::ReloadPlugins(const HTTPRequest*,
                                  HTTPResponse *response) {
  m_client.ReloadPlugins(
      NewSingleCallback(this, &OladHTTPServer::HandleBoolResponse, response));
  return MHD_YES;
}


/**
 * @brief Reload the PID Store.
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::ReloadPidStore(OLA_UNUSED const HTTPRequest *request,
                                   HTTPResponse *response) {
  m_ola_server->ReloadPidStore();
  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->Append("ok");
  int r = response->Send();
  delete response;
  return r;
}


/**
 * @brief Handle the plugin list callback
 * @param response the HTTPResponse that is associated with the request.
 * @param result the result of the API call
 * @param plugins a list of plugins
 */
void OladHTTPServer::HandlePluginList(HTTPResponse *response,
                                      const client::Result &result,
                                      const vector<OlaPlugin> &plugins) {
  if (!result.Success()) {
    m_server.ServeError(response, result.Error());
    return;
  }

  JsonObject *json = new JsonObject();

  // fire off the universe request now. the main server is running in a
  // separate thread.
  m_client.FetchUniverseList(
      NewSingleCallback(this,
                        &OladHTTPServer::HandleUniverseList,
                        response,
                        json));

  JsonArray *plugins_json = json->AddArray("plugins");
  vector<OlaPlugin>::const_iterator iter;
  for (iter = plugins.begin(); iter != plugins.end(); ++iter) {
    JsonObject *plugin = plugins_json->AppendObject();
    plugin->Add("name", iter->Name());
    plugin->Add("id", iter->Id());
    plugin->Add("active", iter->IsActive());
    plugin->Add("enabled", iter->IsEnabled());
  }
}


/**
 * @brief Handle the universe list callback
 * @param response the HTTPResponse that is associated with the request.
 * @param json the JsonObject to add the data to
 * @param result the result of the API call
 * @param universes the vector of OlaUniverse
 */
void OladHTTPServer::HandleUniverseList(HTTPResponse *response,
                                        JsonObject *json,
                                        const client::Result &result,
                                        const vector<OlaUniverse> &universes) {
  if (result.Success()) {
    JsonArray *universe_json = json->AddArray("universes");

    vector<OlaUniverse>::const_iterator iter;
    for (iter = universes.begin(); iter != universes.end(); ++iter) {
      JsonObject *universe = universe_json->AppendObject();
      universe->Add("id", iter->Id());
      universe->Add("input_ports", iter->InputPortCount());
      universe->Add("name", iter->Name());
      universe->Add("output_ports", iter->OutputPortCount());
      universe->Add("rdm_devices", iter->RDMDeviceCount());
    }
  }

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(*json);
  delete response;
  delete json;
}


/**
 * @brief Handle the plugin description response.
 * @param response the HTTPResponse that is associated with the request.
 * @param plugin_id the plugin id.
 * @param result the result of the API call.
 * @param description the plugin description.
 */
void OladHTTPServer::HandlePartialPluginInfo(HTTPResponse *response,
                                             int plugin_id,
                                             const client::Result &result,
                                             const string &description) {
  if (!result.Success()) {
    m_server.ServeError(response, result.Error());
    return;
  }
  m_client.FetchPluginState(
      (ola_plugin_id) plugin_id,
      NewSingleCallback(this,
                        &OladHTTPServer::HandlePluginInfo,
                        response, description));
}

/**
 * @brief Handle the plugin description response.
 * @param response the HTTPResponse that is associated with the request.
 * @param description the plugin description
 * @param result the result of the API call.
 * @param state the state of the plugin.
 */
void OladHTTPServer::HandlePluginInfo(HTTPResponse *response,
                                      string description,
                                      const client::Result &result,
                                      const ola::client::PluginState &state) {
  if (!result.Success()) {
    m_server.ServeError(response, result.Error());
    return;
  }

  string escaped_description = description;
  // Replace \n before passing in so we get \\n out the far end
  ReplaceAll(&escaped_description, "\n", "\\n");

  JsonObject json;
  json.Add("description", escaped_description);
  json.Add("name", state.name);
  json.Add("enabled", state.enabled);
  json.Add("active", state.active);
  json.Add("preferences_source", state.preferences_source);
  JsonArray *plugins = json.AddArray("conflicts_with");
  vector<OlaPlugin>::const_iterator iter = state.conflicting_plugins.begin();
  for (; iter != state.conflicting_plugins.end(); ++iter) {
    JsonObject *plugin = plugins->AppendObject();
    plugin->Add("active", iter->IsActive());
    plugin->Add("id", iter->Id());
    plugin->Add("name", iter->Name());
  }

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete response;
}


/**
 * @brief Handle the universe info
 * @param response the HTTPResponse that is associated with the request.
 * @param result the result of the API call
 * @param universe the OlaUniverse object
 */
void OladHTTPServer::HandleUniverseInfo(HTTPResponse *response,
                                        const client::Result &result,
                                        const OlaUniverse &universe) {
  if (!result.Success()) {
    m_server.ServeError(response, result.Error());
    return;
  }

  JsonObject *json = new JsonObject();

  // fire off the device/port request now. the main server is running in a
  // separate thread.
  m_client.FetchDeviceInfo(
      ola::OLA_PLUGIN_ALL,
      NewSingleCallback(this,
                        &OladHTTPServer::HandlePortsForUniverse,
                        response,
                        json,
                        universe.Id()));

  json->Add("id", universe.Id());
  json->Add("name", universe.Name());
  json->Add("merge_mode",
           (universe.MergeMode() == OlaUniverse::MERGE_HTP ? "HTP" : "LTP"));
}


void OladHTTPServer::HandlePortsForUniverse(
    HTTPResponse *response,
    JsonObject *json,
    unsigned int universe_id,
    const client::Result &result,
    const vector<OlaDevice> &devices) {
  if (result.Success()) {
    vector<OlaDevice>::const_iterator iter = devices.begin();
    vector<OlaInputPort>::const_iterator input_iter;
    vector<OlaOutputPort>::const_iterator output_iter;

    JsonArray *output_ports_json = json->AddArray("output_ports");
    JsonArray *input_ports_json = json->AddArray("input_ports");

    for (; iter != devices.end(); ++iter) {
      const vector<OlaInputPort> &input_ports = iter->InputPorts();
      for (input_iter = input_ports.begin(); input_iter != input_ports.end();
           ++input_iter) {
        if (input_iter->IsActive() && input_iter->Universe() == universe_id) {
          JsonObject *obj = input_ports_json->AppendObject();
          PortToJson(obj, *iter, *input_iter, false);
        }
      }

      const vector<OlaOutputPort> &output_ports = iter->OutputPorts();
      for (output_iter = output_ports.begin();
           output_iter != output_ports.end(); ++output_iter) {
        if (output_iter->IsActive() &&
            output_iter->Universe() == universe_id) {
          JsonObject *obj = output_ports_json->AppendObject();
          PortToJson(obj, *iter, *output_iter, true);
        }
      }
    }
  }

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(*json);
  delete json;
  delete response;
}


/**
 * @brief Handle the list of candidate ports
 * @param response the HTTPResponse that is associated with the request.
 * @param result the result of the API call
 * @param devices the possbile devices & ports
 */
void OladHTTPServer::HandleCandidatePorts(
    HTTPResponse *response,
    const client::Result &result,
    const vector<OlaDevice> &devices) {
  if (!result.Success()) {
    m_server.ServeError(response, result.Error());
    return;
  }

  vector<OlaDevice>::const_iterator iter = devices.begin();
  vector<OlaInputPort>::const_iterator input_iter;
  vector<OlaOutputPort>::const_iterator output_iter;

  JsonArray json;
  for (; iter != devices.end(); ++iter) {
    const vector<OlaInputPort> &input_ports = iter->InputPorts();
    for (input_iter = input_ports.begin(); input_iter != input_ports.end();
         ++input_iter) {
      JsonObject *obj = json.AppendObject();
      PortToJson(obj, *iter, *input_iter, false);
    }

    const vector<OlaOutputPort> &output_ports = iter->OutputPorts();
    for (output_iter = output_ports.begin();
         output_iter != output_ports.end(); ++output_iter) {
      JsonObject *obj = json.AppendObject();
      PortToJson(obj, *iter, *output_iter, true);
    }
  }

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete response;
}


/*
 * @brief Schedule a callback to send the new universe response to the client
 */
void OladHTTPServer::CreateUniverseComplete(HTTPResponse *response,
                                            unsigned int universe_id,
                                            bool included_name,
                                            class ActionQueue *action_queue) {
  // this is a trick to unwind the stack and return control to a method outside
  // the Action
  m_server.SelectServer()->RegisterSingleTimeout(
    0,
    NewSingleCallback(this, &OladHTTPServer::SendCreateUniverseResponse,
                      response, universe_id, included_name, action_queue));
}



/*
 * @brief Send the response to a new universe request
 */
void OladHTTPServer::SendCreateUniverseResponse(
    HTTPResponse *response,
    unsigned int universe_id,
    bool included_name,
    class ActionQueue *action_queue) {
  unsigned int action_count = action_queue->ActionCount();
  if (included_name) {
    action_count--;
  }
  bool failed = true;
  // it only takes one port patch to pass
  for (unsigned int i = 0; i < action_count; i++) {
    failed &= action_queue->GetAction(i)->Failed();
  }

  JsonObject json;
  json.Add("ok", !failed);
  json.Add("universe", universe_id);
  json.Add("message", (failed ? "Failed to patch any ports" : ""));

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete action_queue;
  delete response;
}


/*
 * @brief Schedule a callback to send the modify universe response to the client
 */
void OladHTTPServer::ModifyUniverseComplete(HTTPResponse *response,
                                            ActionQueue *action_queue) {
  // this is a trick to unwind the stack and return control to a method outside
  // the Action
  m_server.SelectServer()->RegisterSingleTimeout(
    0,
    NewSingleCallback(this, &OladHTTPServer::SendModifyUniverseResponse,
                     response, action_queue));
}


/*
 * @brief Send the response to a modify universe request.
 */
void OladHTTPServer::SendModifyUniverseResponse(HTTPResponse *response,
                                                ActionQueue *action_queue) {
  if (!action_queue->WasSuccessful()) {
    delete action_queue;
    m_server.ServeError(response, "Update failed");
  } else {
    response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
    response->Append("ok");
    response->Send();
    delete action_queue;
    delete response;
  }
}

/**
 * @brief Serve usage information.
 * @param response the response to use.
 * @param details the usage information
 */
int OladHTTPServer::ServeUsage(HTTPResponse *response, const string &details) {
  response->SetContentType(HTTPServer::CONTENT_TYPE_HTML);
  response->Append("<b>Usage:</b>");
  if (!details.empty()) {
    response->Append("<p>");
    response->Append(details);
    response->Append("</p>");
  }
  int r = response->Send();
  delete response;
  return r;
}

/**
 * @brief Callback for m_client.FetchDmx called by GetDmx
 * @param response the HTTPResponse
 * @param result the result of the API call
 * @param buffer the DmxBuffer
 */
void OladHTTPServer::HandleGetDmx(HTTPResponse *response,
                                  const client::Result &result,
                                  const client::DMXMetadata &,
                                  const DmxBuffer &buffer) {
  // rather than adding 512 JsonValue we cheat and use raw here
  ostringstream str;
  str << "[" << buffer.ToString() << "]";
  JsonObject json;
  json.AddRaw("dmx", str.str());
  json.Add("error", result.Error());

  response->SetNoCache();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete response;
}


/**
 * @brief Handle the set DMX response.
 * @param response the HTTPResponse that is associated with the request.
 * @param result the result of the API call
 */
void OladHTTPServer::HandleBoolResponse(HTTPResponse *response,
                                        const client::Result &result) {
  if (!result.Success()) {
    m_server.ServeError(response, result.Error());
    return;
  }
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->Append("ok");
  response->Send();
  delete response;
}


/**
 * @brief Add the json representation of this port to the ostringstream
 */
void OladHTTPServer::PortToJson(JsonObject *json,
                                const OlaDevice &device,
                                const OlaPort &port,
                                bool is_output) {
  ostringstream str;
  str << device.Alias() << "-" << (is_output ? "O" : "I") << "-" << port.Id();

  json->Add("device", device.Name());
  json->Add("description", port.Description());
  json->Add("id", str.str());
  json->Add("is_output", is_output);

  JsonObject *priority_json = json->AddObject("priority");
  if (port.PriorityCapability() != CAPABILITY_NONE) {
    // This can be used as the default value for the priority input and because
    // inherit ports can return a 0 priority we shall set it to the default
    // here
    uint8_t priority = port.Priority();
    if (priority == 0) {
      // We check here because 0 is an invalid priority outside of Olad
      priority = dmx::SOURCE_PRIORITY_DEFAULT;
    }
    priority_json->Add("value", static_cast<int>(priority));
    priority_json->Add(
      "current_mode",
      (port.PriorityMode() == PRIORITY_MODE_INHERIT ?  "inherit" : "static"));
    priority_json->Add("priority_capability",
      (port.PriorityCapability() == CAPABILITY_STATIC ? "static" : "full"));
  }
}


/**
 * @brief Add the Patch Actions to the ActionQueue.
 * @param action_queue the ActionQueue to add the actions to.
 * @param port_id_string a string to ports to add/remove.
 * @param universe the universe id to add these ports if
 * @param port_action either PATCH or UNPATCH.
 */
void OladHTTPServer::AddPatchActions(ActionQueue *action_queue,
                                     const string port_id_string,
                                     unsigned int universe,
                                     client::PatchAction port_action) {
  vector<port_identifier> ports;
  vector<port_identifier>::const_iterator iter;
  DecodePortIds(port_id_string, &ports);

  for (iter = ports.begin(); iter != ports.end(); ++iter) {
    action_queue->AddAction(new PatchPortAction(
      &m_client,
      iter->device_alias,
      iter->port,
      iter->direction,
      universe,
      port_action));
  }
}


/**
 * @brief Add the Priority Actions to the ActionQueue.
 * @param action_queue the ActionQueue to add the actions to.
 * @param request the HTTPRequest to read the url params from.
 */
void OladHTTPServer::AddPriorityActions(ActionQueue *action_queue,
                                        const HTTPRequest *request) {
  string port_ids = request->GetPostParameter("modify_ports");
  vector<port_identifier> ports;
  vector<port_identifier>::const_iterator iter;
  DecodePortIds(port_ids, &ports);

  for (iter = ports.begin(); iter != ports.end(); ++iter) {
    string priority_mode_id = iter->string_id + K_PRIORITY_MODE_SUFFIX;
    string priority_id = iter->string_id + K_PRIORITY_VALUE_SUFFIX;
    string mode = request->GetPostParameter(priority_mode_id);

    if (mode == "inherit") {
      action_queue->AddAction(new PortPriorityInheritAction(
        &m_client,
        iter->device_alias,
        iter->port,
        iter->direction));
    } else if (mode == "static" || mode == "") {
      // an empty mode param means this is a static port
      string value = request->GetPostParameter(priority_id);
      uint8_t priority_value;
      if (StringToInt(value, &priority_value)) {
        action_queue->AddAction(new PortPriorityStaticAction(
          &m_client,
          iter->device_alias,
          iter->port,
          iter->direction,
          priority_value));
      }
    }
  }
}


/**
 * @brief Decode port ids in a string.
 *
 * This converts a string like "4-I-1,2-O-3" into a vector of port identifiers.
 * @param port_ids the port ids in a , separated string
 * @param ports a vector of port_identifiers that will be filled.
 */
void OladHTTPServer::DecodePortIds(const string &port_ids,
                                   vector<port_identifier> *ports) {
  vector<string> port_strings;
  StringSplit(port_ids, &port_strings, ",");
  vector<string>::const_iterator iter;
  vector<string> tokens;

  for (iter = port_strings.begin(); iter != port_strings.end(); ++iter) {
    if (iter->empty()) {
      continue;
    }

    tokens.clear();
    StringSplit(*iter, &tokens, "-");

    if (tokens.size() != 3 || (tokens[1] != "I" && tokens[1] != "O")) {
      OLA_INFO << "Not a valid port id " << *iter;
      continue;
    }

    unsigned int device_alias, port;
    if (!StringToInt(tokens[0], &device_alias) ||
        !StringToInt(tokens[2], &port)) {
      OLA_INFO << "Not a valid port id " << *iter;
      continue;
    }

    client::PortDirection direction = (
        tokens[1] == "I" ?  client::INPUT_PORT : client::OUTPUT_PORT);
    port_identifier port_id = {device_alias, port, direction, *iter};
    ports->push_back(port_id);
  }
}


/**
 * @brief Register a handler
 */
inline void OladHTTPServer::RegisterHandler(
    const string &path,
    int (OladHTTPServer::*method)(const HTTPRequest*, HTTPResponse*)) {
  m_server.RegisterHandler(
      path,
      NewCallback<OladHTTPServer, int, const HTTPRequest*, HTTPResponse*>(
        this,
        method));
}
}  // namespace ola
