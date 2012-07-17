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
 * OlaHttpServer.cpp
 * Ola HTTP class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <sys/time.h>
#include <iostream>
#include <string>
#include <vector>

#include "ola/ActionQueue.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "olad/DmxSource.h"
#include "olad/HttpServerActions.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/network/NetworkUtils.h"
#include "olad/OlaHttpServer.h"
#include "olad/OlaServer.h"
#include "olad/OlaVersion.h"


namespace ola {

using ola::io::ConnectedDescriptor;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::vector;

const char OlaHttpServer::K_DATA_DIR_VAR[] = "http_data_dir";
const char OlaHttpServer::K_UPTIME_VAR[] = "uptime-in-ms";
const char OlaHttpServer::K_BACKEND_DISCONNECTED_ERROR[] =
  "Failed to send request, client isn't connected";
const char OlaHttpServer::K_PRIORITY_VALUE_SUFFIX[] = "_priority_value";
const char OlaHttpServer::K_PRIORITY_MODE_SUFFIX[] = "_priority_mode";


/**
 * Create a new OLA HTTP server
 * @param export_map the ExportMap to display when /debug is called
 * @param client_socket A ConnectedDescriptor which is used to communicate with the
 *   server.
 * @param
 */
OlaHttpServer::OlaHttpServer(ExportMap *export_map,
                             ConnectedDescriptor *client_socket,
                             OlaServer *ola_server,
                             unsigned int port,
                             bool enable_quit,
                             const string &data_dir,
                             const ola::network::Interface &interface)
    : m_server(port, data_dir),
      m_export_map(export_map),
      m_client_socket(client_socket),
      m_client(client_socket),
      m_ola_server(ola_server),
      m_enable_quit(enable_quit),
      m_interface(interface),
      m_rdm_module(&m_server, &m_client) {
  // The main handlers
  RegisterHandler("/", &OlaHttpServer::DisplayIndex);
  RegisterHandler("/debug", &OlaHttpServer::DisplayDebug);
  RegisterHandler("/help", &OlaHttpServer::DisplayHandlers);
  RegisterHandler("/quit", &OlaHttpServer::DisplayQuit);
  RegisterHandler("/reload", &OlaHttpServer::ReloadPlugins);
  RegisterHandler("/new_universe", &OlaHttpServer::CreateNewUniverse);
  RegisterHandler("/modify_universe", &OlaHttpServer::ModifyUniverse);
  RegisterHandler("/set_dmx", &OlaHttpServer::HandleSetDmx);
  RegisterHandler("/get_dmx", &OlaHttpServer::GetDmx);

  // json endpoints for the new UI
  RegisterHandler("/json/server_stats", &OlaHttpServer::JsonServerStats);
  RegisterHandler("/json/universe_plugin_list",
                  &OlaHttpServer::JsonUniversePluginList);
  RegisterHandler("/json/plugin_info", &OlaHttpServer::JsonPluginInfo);
  RegisterHandler("/json/get_ports", &OlaHttpServer::JsonAvailablePorts);
  RegisterHandler("/json/universe_info", &OlaHttpServer::JsonUniverseInfo);

  // these are the static files for the new UI
  RegisterFile("blank.gif", HttpServer::CONTENT_TYPE_GIF);
  RegisterFile("button-bg.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("custombutton.css", HttpServer::CONTENT_TYPE_CSS);
  RegisterFile("editortoolbar.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("expander.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("handle.vertical.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("loader.gif", HttpServer::CONTENT_TYPE_GIF);
  RegisterFile("loader-mini.gif", HttpServer::CONTENT_TYPE_GIF);
  RegisterFile("logo.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("logo-mini.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("mobile.html", HttpServer::CONTENT_TYPE_HTML);
  RegisterFile("mobile.js", HttpServer::CONTENT_TYPE_JS);
  RegisterFile("ola.html", HttpServer::CONTENT_TYPE_HTML);
  RegisterFile("ola.js", HttpServer::CONTENT_TYPE_JS);
  RegisterFile("tick.gif", HttpServer::CONTENT_TYPE_GIF);
  RegisterFile("toolbar-bg.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("toolbar.css", HttpServer::CONTENT_TYPE_CSS);
  RegisterFile("toolbar_sprites.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("vertical.gif", HttpServer::CONTENT_TYPE_GIF);

  StringVariable *data_dir_var = export_map->GetStringVar(K_DATA_DIR_VAR);
  data_dir_var->Set(m_server.DataDir());
  m_clock.CurrentTime(&m_start_time);
  m_start_time_t = time(NULL);
  export_map->GetStringVar(K_UPTIME_VAR);
}


/*
 * Teardown
 */
OlaHttpServer::~OlaHttpServer() {
  if (m_client_socket)
    m_server.SelectServer()->RemoveReadDescriptor(m_client_socket);
  m_client.Stop();
  if (m_client_socket)
    delete m_client_socket;
}


/**
 * Setup the OLA HTTP server
 * @return true if this worked, false otherwise.
 */
bool OlaHttpServer::Init() {
  bool ret = m_server.Init();
  if (ret) {
    if (!m_client.Setup()) {
      return false;
    }
    /*
    Setup disconnect notifications.
    m_socket->SetOnClose(
      ola::NewSingleCallback(this, &SimpleClient::SocketClosed));
    */
    m_server.SelectServer()->AddReadDescriptor(m_client_socket);
  }
  return ret;
}


/*
 * Print the server stats json
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::JsonServerStats(const HttpRequest *request,
                                   HttpResponse *response) {
  struct tm start_time;
  char start_time_str[50];
  localtime_r(&m_start_time_t, &start_time);
  strftime(start_time_str, sizeof(start_time_str), "%c", &start_time);

  stringstream str;
  str << "{" << endl;
  str << "  \"hostname\": \"" << EscapeString(ola::network::FullHostname()) <<
    "\"," << endl;
  str << "  \"ip\": \"" << m_interface.ip_address << "\"," << endl;
  str << "  \"broadcast\": \"" << m_interface.bcast_address << "\"," << endl;
  str << "  \"subnet\": \"" << m_interface.subnet_mask << "\"," << endl;
  str << "  \"hw_address\": \"" <<
    ola::network::HardwareAddressToString(m_interface.hw_address) << "\","
    << endl;
  str << "  \"version\": \"" << OLA_VERSION << "\"," << endl;
  str << "  \"up_since\": \"" << start_time_str << "\"," << endl;
  str << "  \"quit_enabled\": " << m_enable_quit << "," << endl;
  str << "}";

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  int r = response->Send();
  delete response;
  return r;
  (void) request;
}


/*
 * Print the list of universes / plugins as a json string
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::JsonUniversePluginList(const HttpRequest *request,
                                          HttpResponse *response) {
  bool ok = m_client.FetchPluginList(
      NewSingleCallback(this,
                        &OlaHttpServer::HandlePluginList,
                        response));

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
  (void) request;
}


/**
 * Print the plugin info as a json string
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::JsonPluginInfo(const HttpRequest *request,
                                  HttpResponse *response) {
  string val = request->GetParameter("id");
  int plugin_id = atoi(val.data());

  bool ok = m_client.FetchPluginDescription(
      (ola_plugin_id) plugin_id,
      NewSingleCallback(this,
                        &OlaHttpServer::HandlePluginInfo,
                        response));

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/**
 * Return information about a universe
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::JsonUniverseInfo(const HttpRequest *request,
                                    HttpResponse *response) {
  string uni_id = request->GetParameter("id");
  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  bool ok = m_client.FetchUniverseInfo(
      universe_id,
      NewSingleCallback(this,
                        &OlaHttpServer::HandleUniverseInfo,
                        response));

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
  (void) request;
}


/**
 * Return a list of unbound ports
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::JsonAvailablePorts(const HttpRequest *request,
                                      HttpResponse *response) {
  string uni_id = request->GetParameter("id");
  bool ok = false;

  if (uni_id.empty()) {
    // get all available ports
    ok = m_client.FetchCandidatePorts(
        NewSingleCallback(this,
                          &OlaHttpServer::HandleCandidatePorts,
                          response));
  } else {
    unsigned int universe_id;
    if (!StringToInt(uni_id, &universe_id))
      return m_server.ServeNotFound(response);

    ok = m_client.FetchCandidatePorts(
        universe_id,
        NewSingleCallback(this,
                          &OlaHttpServer::HandleCandidatePorts,
                          response));
  }

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/*
 * Create a new universe by binding one or more ports.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::CreateNewUniverse(const HttpRequest *request,
                                     HttpResponse *response) {
  string uni_id = request->GetPostParameter("id");
  string name = request->GetPostParameter("name");

  if (name.size() > K_UNIVERSE_NAME_LIMIT)
    name = name.substr(K_UNIVERSE_NAME_LIMIT);

  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  ActionQueue *action_queue = new ActionQueue(
      NewSingleCallback(this,
                        &OlaHttpServer::CreateUniverseComplete,
                        response,
                        universe_id,
                        !name.empty()));

  // add patch actions here
  string add_port_ids = request->GetPostParameter("add_ports");
  AddPatchActions(action_queue, add_port_ids, universe_id, PATCH);


  if (!name.empty())
    action_queue->AddAction(
        new SetNameAction(&m_client, universe_id, name, false));

  action_queue->NextAction();
  return MHD_YES;
}


/*
 * Modify an existing universe.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::ModifyUniverse(const HttpRequest *request,
                                  HttpResponse *response) {
  string uni_id = request->GetPostParameter("id");
  string name = request->GetPostParameter("name");
  string merge_mode = request->GetPostParameter("merge_mode");

  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  if (name.empty())
    return m_server.ServeError(response, "No name supplied");

  if (name.size() > K_UNIVERSE_NAME_LIMIT)
    name = name.substr(K_UNIVERSE_NAME_LIMIT);

  ActionQueue *action_queue = new ActionQueue(
      NewSingleCallback(this,
                        &OlaHttpServer::ModifyUniverseComplete,
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
  AddPatchActions(action_queue, remove_port_ids, universe_id, UNPATCH);

  string add_port_ids = request->GetPostParameter("add_ports");
  AddPatchActions(action_queue, add_port_ids, universe_id, PATCH);

  AddPriorityActions(action_queue, request);

  action_queue->NextAction();
  return MHD_YES;
}


/*
 * Display the index page
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::DisplayIndex(const HttpRequest *request,
                                HttpResponse *response) {
  HttpServer::static_file_info file_info;
  file_info.file_path = "landing.html";
  file_info.content_type = HttpServer::CONTENT_TYPE_HTML;
  return m_server.ServeStaticContent(&file_info, response);
  (void) request;
}


/*
 * Handle the get dmx command
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::GetDmx(const HttpRequest *request,
                          HttpResponse *response) {
  string uni_id = request->GetParameter("u");
  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);
  int ok = m_client.FetchDmx(universe_id,
                             NewSingleCallback(this,
                                               &OlaHttpServer::HandleGetDmx,
                                               response));

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);

  return MHD_YES;
}

/*
 * Handle the set dmx command
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::HandleSetDmx(const HttpRequest *request,
                                HttpResponse *response) {
  string dmx_data_str = request->GetPostParameter("d");
  string uni_id = request->GetPostParameter("u");
  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  DmxBuffer buffer;
  buffer.SetFromString(dmx_data_str);
  if (!buffer.Size())
    return m_server.ServeError(response, "Invalid DMX string");

  bool ok = m_client.SendDmx(
      universe_id,
      buffer,
      NewSingleCallback(this, &OlaHttpServer::HandleBoolResponse, response));

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/*
 * Display the debug page
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::DisplayDebug(const HttpRequest *request,
                                HttpResponse *response) {
  TimeStamp now;
  m_clock.CurrentTime(&now);
  TimeInterval diff = now - m_start_time;
  stringstream str;
  str << (diff.AsInt() / 1000);
  m_export_map->GetStringVar(K_UPTIME_VAR)->Set(str.str());

  vector<BaseVariable*> variables = m_export_map->AllVariables();
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);

  vector<BaseVariable*>::iterator iter;
  for (iter = variables.begin(); iter != variables.end(); ++iter) {
    stringstream out;
    out << (*iter)->Name() << ": " << (*iter)->Value() << "\n";
    response->Append(out.str());
  }
  int r = response->Send();
  delete response;
  return r;
  (void) request;
}


/*
 * Cause the server to shutdown
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::DisplayQuit(const HttpRequest *request,
                               HttpResponse *response) {
  if (m_enable_quit) {
    response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
    response->Append("ok");
    m_ola_server->StopServer();
  } else {
    response->SetStatus(403);
    response->SetContentType(HttpServer::CONTENT_TYPE_HTML);
    response->Append("<b>403 Unauthorized</b>");
  }
  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  int r = response->Send();
  delete response;
  return r;
  (void) request;
}


/*
 * Reload all plugins
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::ReloadPlugins(const HttpRequest *request,
                                 HttpResponse *response) {
  m_ola_server->ReloadPlugins();
  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append("ok");
  int r = response->Send();
  delete response;
  return r;
  (void) request;
}


/*
 * Display a list of registered handlers
 */
int OlaHttpServer::DisplayHandlers(const HttpRequest *request,
                                   HttpResponse *response) {
  vector<string> handlers = m_server.Handlers();
  vector<string>::const_iterator iter;
  response->SetContentType(HttpServer::CONTENT_TYPE_HTML);
  response->Append("<html><body><b>Registered Handlers</b><ul>");
  for (iter = handlers.begin(); iter != handlers.end(); ++iter) {
    response->Append("<li><a href='" + *iter + "'>" + *iter + "</a></li>");
  }
  response->Append("</ul></body></html>");
  int r = response->Send();
  delete response;
  return r;
  (void) request;
}


/*
 * Handle the plugin list callback
 * @param response the HttpResponse that is associated with the request.
 * @param plugins a list of plugins
 * @param error an error string.
 */
void OlaHttpServer::HandlePluginList(HttpResponse *response,
                                     const vector<OlaPlugin> &plugins,
                                     const string &error) {
  if (!error.empty()) {
    m_server.ServeError(response, error);
    return;
  }

  // fire off the universe request now. the main server is running in a
  // separate thread.
  bool ok = m_client.FetchUniverseList(
      NewSingleCallback(this,
                        &OlaHttpServer::HandleUniverseList,
                        response));

  if (!ok) {
    m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
    return;
  }

  stringstream str;
  str << "{" << endl;
  str << "  \"plugins\": [" << endl;

  vector<OlaPlugin>::const_iterator iter;
  string delim = "";
  for (iter = plugins.begin(); iter != plugins.end(); ++iter) {
    str << delim << "    {\"name\": \"" << EscapeString(iter->Name()) <<
      "\", \"id\": " << iter->Id() << "}";
    delim = ",\n";
  }
  str << endl;
  str << "  ]," << endl;
  response->Append(str.str());
}


/*
 * Handle the universe list callback
 * @param response the HttpResponse that is associated with the request.
 * @param plugins a list of plugins
 * @param error an error string.
 */
void OlaHttpServer::HandleUniverseList(HttpResponse *response,
                                       const vector<OlaUniverse> &universes,
                                       const string &error) {
  stringstream str;
  if (error.empty()) {
    str << "  \"universes\": [" << endl;

    vector<OlaUniverse>::const_iterator iter;
    string delim = "";

    for (iter = universes.begin(); iter != universes.end(); ++iter) {
      str << delim;
      str << "    {" << endl;
      str << "      \"id\": " << iter->Id() << "," << endl;
      str << "      \"input_ports\": " << iter->InputPortCount() << "," <<
        endl;
      str << "      \"name\": \"" << EscapeString(iter->Name()) << "\"," <<
        endl;
      str << "      \"output_ports\": " << iter->OutputPortCount() << "," <<
        endl;
      str << "      \"rdm_devices\": " << iter->RDMDeviceCount() << "," <<
        endl;
      str << "    }";
      delim = ",\n";
    }
    str << endl << "  ]," << endl;
  }
  str << "}";

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;
}


/*
 * Handle the plugin description response.
 * @param response the HttpResponse that is associated with the request.
 * @param description the plugin description.
 * @param error an error string.
 */
void OlaHttpServer::HandlePluginInfo(HttpResponse *response,
                                     const string &description,
                                     const string &error) {
  if (!error.empty()) {
    m_server.ServeError(response, error);
    return;
  }
  string escaped_description = description;
  Escape(&escaped_description);

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append("{\"description\": \"");
  response->Append(escaped_description);
  response->Append("\"}");
  response->Send();
  delete response;
}


/*
 * Handle the universe info
 * @param response the HttpResponse that is associated with the request.
 * @param universe the OlaUniverse object
 * @param error an error string.
 */
void OlaHttpServer::HandleUniverseInfo(HttpResponse *response,
                                       OlaUniverse &universe,
                                       const string &error) {
  if (!error.empty()) {
    m_server.ServeError(response, error);
    return;
  }

  // fire off the device/port request now. the main server is running in a
  // separate thread.
  bool ok = m_client.FetchDeviceInfo(
      ola::OLA_PLUGIN_ALL,
      NewSingleCallback(this,
                        &OlaHttpServer::HandlePortsForUniverse,
                        response,
                        universe.Id()));

  if (!ok) {
    m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
    return;
  }

  stringstream str;
  str << "{" << endl;
  str << "  \"id\": " << universe.Id() << "," << endl;
  str << "  \"name\": \"" << EscapeString(universe.Name()) << "\"," << endl;
  str << "  \"merge_mode\": \"" <<
    (universe.MergeMode() == OlaUniverse::MERGE_HTP ? "HTP" : "LTP") << "\","
    << endl;

  response->Append(str.str());
}


void OlaHttpServer::HandlePortsForUniverse(
    HttpResponse *response,
    unsigned int universe_id,
    const vector<OlaDevice> &devices,
    const string &error) {
  if (error.empty()) {
    stringstream input_str, output_str;
    vector<OlaDevice>::const_iterator iter = devices.begin();
    vector<OlaInputPort>::const_iterator input_iter;
    vector<OlaOutputPort>::const_iterator output_iter;

    input_str << "  \"input_ports\": [" << endl;
    output_str << "  \"output_ports\": [" << endl;
    for (; iter != devices.end(); ++iter) {
      const vector<OlaInputPort> &input_ports = iter->InputPorts();
      for (input_iter = input_ports.begin(); input_iter != input_ports.end();
           ++input_iter) {
        if (input_iter->IsActive() && input_iter->Universe() == universe_id)
          PortToJson(*iter, *input_iter, &input_str, false);
      }

      const vector<OlaOutputPort> &output_ports = iter->OutputPorts();
      for (output_iter = output_ports.begin();
           output_iter != output_ports.end(); ++output_iter) {
        if (output_iter->IsActive() && output_iter->Universe() == universe_id)
          PortToJson(*iter, *output_iter, &output_str, true);
      }
    }
    input_str << "  ]," << endl;
    output_str << "  ]," << endl;
    response->Append(input_str.str());
    response->Append(output_str.str());
  }

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append("}");
  response->Send();
  delete response;
}


/*
 * Handle the list of candidate ports
 * @param response the HttpResponse that is associated with the request.
 * @param devices the possbile devices & ports
 * @param error an error string.
 */
void OlaHttpServer::HandleCandidatePorts(
    HttpResponse *response,
    const vector<class OlaDevice> &devices,
    const string &error) {
  if (!error.empty()) {
    m_server.ServeError(response, error);
    return;
  }

  stringstream str;
  str << "[" << endl;

  vector<OlaDevice>::const_iterator iter = devices.begin();
  vector<OlaInputPort>::const_iterator input_iter;
  vector<OlaOutputPort>::const_iterator output_iter;

  for (; iter != devices.end(); ++iter) {
    const vector<OlaInputPort> &input_ports = iter->InputPorts();
    for (input_iter = input_ports.begin(); input_iter != input_ports.end();
         ++input_iter) {
      PortToJson(*iter, *input_iter, &str, false);
    }

    const vector<OlaOutputPort> &output_ports = iter->OutputPorts();
    for (output_iter = output_ports.begin();
         output_iter != output_ports.end(); ++output_iter) {
      PortToJson(*iter, *output_iter, &str, true);
    }
  }
  str << "]" << endl;

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;
}


/*
 * Schedule a callback to send the new universe response to the client
 */
void OlaHttpServer::CreateUniverseComplete(HttpResponse *response,
                                           unsigned int universe_id,
                                           bool included_name,
                                           class ActionQueue *action_queue) {
  // this is a trick to unwind the stack and return control to a method outside
  // the Action
  m_server.SelectServer()->RegisterSingleTimeout(
    0,
    NewSingleCallback(this, &OlaHttpServer::SendCreateUniverseResponse,
                      response, universe_id, included_name, action_queue));
}



/*
 * Send the response to a new universe request
 */
void OlaHttpServer::SendCreateUniverseResponse(
    HttpResponse *response,
    unsigned int universe_id,
    bool included_name,
    class ActionQueue *action_queue) {
  unsigned int action_count = action_queue->ActionCount();
  if (included_name)
    action_count--;
  bool failed = true;
  // it only takes one port patch to pass
  for (unsigned int i = 0; i < action_count; i++) {
    failed &= action_queue->GetAction(i)->Failed();
  }

  stringstream str;
  str << "{" << endl;
  str << "  \"ok\": " << !failed << "," << endl;
  str << "  \"universe\": " << universe_id << "," << endl;
  str << "  \"message\": \"" << (failed ? "Failed to patch any ports" : "") <<
    "\"," << endl;
  str << "}";

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete action_queue;
  delete response;
}


/*
 * Schedule a callback to send the modify universe response to the client
 */
void OlaHttpServer::ModifyUniverseComplete(HttpResponse *response,
                                           ActionQueue *action_queue) {
  // this is a trick to unwind the stack and return control to a method outside
  // the Action
  m_server.SelectServer()->RegisterSingleTimeout(
    0,
    NewSingleCallback(this, &OlaHttpServer::SendModifyUniverseResponse,
                     response, action_queue));
}


/*
 * Send the response to a modify universe request.
 */
void OlaHttpServer::SendModifyUniverseResponse(HttpResponse *response,
                                               ActionQueue *action_queue) {
  if (!action_queue->WasSuccessful()) {
    delete action_queue;
    m_server.ServeError(response, "Update failed");
  } else {
    response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
    response->Append("ok");
    response->Send();
    delete action_queue;
    delete response;
  }
}


/*
 * Callback for m_client.FetchDmx called by GetDmx
 * @param response the HttpResponse
 * @param buffer the DmxBuffer
 * @param error Error message
 */
void OlaHttpServer::HandleGetDmx(HttpResponse *response,
                                 const DmxBuffer &buffer,
                                 const string &error) {
  stringstream str;
  str << "{" << endl;
  str << "  \"dmx\": [" << buffer.ToString() << "]," << endl;
  str << "  \"error\": \"" << error << "\"" << endl;
  str << "}";

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;
}


/*
 * Handle the set DMX response.
 * @param response the HttpResponse that is associated with the request.
 * @param error an error string.
 */
void OlaHttpServer::HandleBoolResponse(HttpResponse *response,
                                       const string &error) {
  if (!error.empty()) {
    m_server.ServeError(response, error);
    return;
  }
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append("ok");
  response->Send();
  delete response;
}


/*
 * Register a handler
 */
inline void OlaHttpServer::RegisterHandler(
    const string &path,
    int (OlaHttpServer::*method)(const HttpRequest*, HttpResponse*)) {
  m_server.RegisterHandler(
      path,
      NewCallback<OlaHttpServer, int, const HttpRequest*, HttpResponse*>(
        this,
        method));
}


/*
 * Register a static file
 */
inline void OlaHttpServer::RegisterFile(const string &file,
                                 const string &content_type) {
  m_server.RegisterFile("/" + file, file, content_type);
}


/**
 * Add the json representation of this port to the stringstream
 */
void OlaHttpServer::PortToJson(const OlaDevice &device,
                               const OlaPort &port,
                               stringstream *str,
                               bool is_output,
                               bool include_delim) {
  *str << "    {" << endl;
  *str << "      \"device\": \"" << EscapeString(device.Name())
    << "\"," << endl;
  *str << "      \"description\": \"" <<
    EscapeString(port.Description()) << "\"," << endl;
  *str << "      \"id\": \"" << device.Alias() << "-" <<
    (is_output ? "O" : "I") << "-" << port.Id() << "\"," << endl;
  *str << "      \"is_output\": " << (is_output ? "true" : "false") << "," <<
    endl;

  if (port.PriorityCapability() != CAPABILITY_NONE) {
    *str << "      \"priority\": {" << endl;
    *str << "        \"value\": " << static_cast<int>(port.Priority()) <<
      "," << endl;
    if (port.PriorityCapability() == CAPABILITY_FULL) {
      *str << "        \"current_mode\": \"" <<
        (port.PriorityMode() == PRIORITY_MODE_INHERIT ?
         "inherit" : "override") << "\"," <<
        endl;
    }
    *str << "      }" << endl;
  }
  *str << "    }" << (include_delim ? "<" : "") << endl;
}


/*
 * Add the Patch Actions to the ActionQueue.
 * @param action_queue the ActionQueue to add the actions to.
 * @param port_id_string a string to ports to add/remove.
 * @param universe the universe id to add these ports if
 * @param port_action either PATCH or UNPATCH.
 */
void OlaHttpServer::AddPatchActions(ActionQueue *action_queue,
                                    const string port_id_string,
                                    unsigned int universe,
                                    PatchAction port_action) {
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


/*
 * Add the Priority Actions to the ActionQueue.
 * @param action_queue the ActionQueue to add the actions to.
 * @param request the HttpRequest to read the url params from.
 */
void OlaHttpServer::AddPriorityActions(ActionQueue *action_queue,
                                       const HttpRequest *request) {
  string port_ids = request->GetPostParameter("modify_ports");
  vector<port_identifier> ports;
  vector<port_identifier>::const_iterator iter;
  DecodePortIds(port_ids, &ports);

  for (iter = ports.begin(); iter != ports.end(); ++iter) {
    string priority_mode_id = iter->string_id + K_PRIORITY_MODE_SUFFIX;
    string priority_id = iter->string_id + K_PRIORITY_VALUE_SUFFIX;
    string mode = request->GetPostParameter(priority_mode_id);

    if (mode == "0") {
      action_queue->AddAction(new PortPriorityInheritAction(
        &m_client,
        iter->device_alias,
        iter->port,
        iter->direction));
    } else if (mode == "1" || mode == "") {
      // an empty mode param means this is a static port
      string value = request->GetPostParameter(priority_id);
      uint8_t priority_value;
      if (StringToInt(value, &priority_value)) {
        action_queue->AddAction(new PortPriorityOverrideAction(
          &m_client,
          iter->device_alias,
          iter->port,
          iter->direction,
          priority_value));
      }
    }
  }
}


/*
 * Decode port ids in a string.
 * This converts a string like "4-I-1,2-O-3" into a vector of port identifiers.
 * @param port_ids the port ids in a , separated string
 * @param ports a vector of port_identifiers that will be filled.
 */
void OlaHttpServer::DecodePortIds(const string &port_ids,
                                  vector<port_identifier> *ports) {
  vector<string> port_strings;
  StringSplit(port_ids, port_strings, ",");
  vector<string>::const_iterator iter;
  vector<string> tokens;

  for (iter = port_strings.begin(); iter != port_strings.end(); ++iter) {
    if (iter->empty())
      continue;

    tokens.clear();
    StringSplit(*iter, tokens, "-");

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

    PortDirection direction = tokens[1] == "I" ? INPUT_PORT : OUTPUT_PORT;
    port_identifier port_id = {device_alias, port, direction, *iter};
    ports->push_back(port_id);
  }
}
}  // ola
