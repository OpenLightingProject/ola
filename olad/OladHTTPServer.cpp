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
 * OladHTTPServer.cpp
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
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/network/NetworkUtils.h"
#include "ola/web/Json.h"
#include "olad/DmxSource.h"
#include "olad/HTTPServerActions.h"
#include "olad/OladHTTPServer.h"
#include "olad/OlaServer.h"
#include "olad/OlaVersion.h"


namespace ola {

using ola::io::ConnectedDescriptor;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::vector;
using ola::web::JsonArray;
using ola::web::JsonObject;

const char OladHTTPServer::K_BACKEND_DISCONNECTED_ERROR[] =
  "Failed to send request, client isn't connected";
const char OladHTTPServer::K_PRIORITY_VALUE_SUFFIX[] = "_priority_value";
const char OladHTTPServer::K_PRIORITY_MODE_SUFFIX[] = "_priority_mode";


/**
 * Create a new OLA HTTP server
 * @param export_map the ExportMap to display when /debug is called
 * @param client_socket A ConnectedDescriptor which is used to communicate with
 *   the server.
 * @param
 */
OladHTTPServer::OladHTTPServer(ExportMap *export_map,
                               const OladHTTPServerOptions &options,
                               ConnectedDescriptor *client_socket,
                               OlaServer *ola_server,
                               const ola::network::Interface &interface)
    : OlaHTTPServer(options, export_map),
      m_client_socket(client_socket),
      m_client(client_socket),
      m_ola_server(ola_server),
      m_enable_quit(options.enable_quit),
      m_interface(interface),
      m_rdm_module(&m_server, &m_client) {
  // The main handlers
  RegisterHandler("/quit", &OladHTTPServer::DisplayQuit);
  RegisterHandler("/reload", &OladHTTPServer::ReloadPlugins);
  RegisterHandler("/new_universe", &OladHTTPServer::CreateNewUniverse);
  RegisterHandler("/modify_universe", &OladHTTPServer::ModifyUniverse);
  RegisterHandler("/set_dmx", &OladHTTPServer::HandleSetDmx);
  RegisterHandler("/get_dmx", &OladHTTPServer::GetDmx);

  // json endpoints for the new UI
  RegisterHandler("/json/server_stats", &OladHTTPServer::JsonServerStats);
  RegisterHandler("/json/universe_plugin_list",
                  &OladHTTPServer::JsonUniversePluginList);
  RegisterHandler("/json/plugin_info", &OladHTTPServer::JsonPluginInfo);
  RegisterHandler("/json/get_ports", &OladHTTPServer::JsonAvailablePorts);
  RegisterHandler("/json/universe_info", &OladHTTPServer::JsonUniverseInfo);

  // these are the static files for the new UI
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
  m_server.RegisterFile("/", "landing.html", HTTPServer::CONTENT_TYPE_HTML);
}


/*
 * Teardown
 */
OladHTTPServer::~OladHTTPServer() {
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
bool OladHTTPServer::Init() {
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
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::JsonServerStats(const HTTPRequest*,
                                   HTTPResponse *response) {
  struct tm start_time;
  char start_time_str[50];
  localtime_r(&m_start_time_t, &start_time);
  strftime(start_time_str, sizeof(start_time_str), "%c", &start_time);

  JsonObject json;
  json.Add("hostname", ola::network::FullHostname());
  json.Add("ip", m_interface.ip_address.ToString());
  json.Add("broadcast", m_interface.bcast_address.ToString());
  json.Add("subnet", m_interface.subnet_mask.ToString());
  json.Add("hw_address",
      ola::network::HardwareAddressToString(m_interface.hw_address));
  json.Add("version", OLA_VERSION);
  json.Add("up_since", start_time_str);
  json.Add("quit_enabled", m_enable_quit);

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  int r = response->SendJson(json);
  delete response;
  return r;
}


/*
 * Print the list of universes / plugins as a json string
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::JsonUniversePluginList(const HTTPRequest*,
                                          HTTPResponse *response) {
  bool ok = m_client.FetchPluginList(
      NewSingleCallback(this,
                        &OladHTTPServer::HandlePluginList,
                        response));

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/**
 * Print the plugin info as a json string
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::JsonPluginInfo(const HTTPRequest *request,
                                  HTTPResponse *response) {
  string val = request->GetParameter("id");
  int plugin_id = atoi(val.data());

  bool ok = m_client.FetchPluginDescription(
      (ola_plugin_id) plugin_id,
      NewSingleCallback(this,
                        &OladHTTPServer::HandlePluginInfo,
                        response));

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/**
 * Return information about a universe
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::JsonUniverseInfo(const HTTPRequest *request,
                                    HTTPResponse *response) {
  string uni_id = request->GetParameter("id");
  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  bool ok = m_client.FetchUniverseInfo(
      universe_id,
      NewSingleCallback(this,
                        &OladHTTPServer::HandleUniverseInfo,
                        response));

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
  (void) request;
}


/**
 * Return a list of unbound ports
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::JsonAvailablePorts(const HTTPRequest *request,
                                      HTTPResponse *response) {
  string uni_id = request->GetParameter("id");
  bool ok = false;

  if (uni_id.empty()) {
    // get all available ports
    ok = m_client.FetchCandidatePorts(
        NewSingleCallback(this,
                          &OladHTTPServer::HandleCandidatePorts,
                          response));
  } else {
    unsigned int universe_id;
    if (!StringToInt(uni_id, &universe_id))
      return m_server.ServeNotFound(response);

    ok = m_client.FetchCandidatePorts(
        universe_id,
        NewSingleCallback(this,
                          &OladHTTPServer::HandleCandidatePorts,
                          response));
  }

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/*
 * Create a new universe by binding one or more ports.
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::CreateNewUniverse(const HTTPRequest *request,
                                     HTTPResponse *response) {
  string uni_id = request->GetPostParameter("id");
  string name = request->GetPostParameter("name");

  if (name.size() > K_UNIVERSE_NAME_LIMIT)
    name = name.substr(K_UNIVERSE_NAME_LIMIT);

  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  ActionQueue *action_queue = new ActionQueue(
      NewSingleCallback(this,
                        &OladHTTPServer::CreateUniverseComplete,
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
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::ModifyUniverse(const HTTPRequest *request,
                                  HTTPResponse *response) {
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
  AddPatchActions(action_queue, remove_port_ids, universe_id, UNPATCH);

  string add_port_ids = request->GetPostParameter("add_ports");
  AddPatchActions(action_queue, add_port_ids, universe_id, PATCH);

  AddPriorityActions(action_queue, request);

  action_queue->NextAction();
  return MHD_YES;
}


/*
 * Handle the get dmx command
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::GetDmx(const HTTPRequest *request,
                          HTTPResponse *response) {
  string uni_id = request->GetParameter("u");
  unsigned int universe_id;
  if (!StringToInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);
  int ok = m_client.FetchDmx(universe_id,
                             NewSingleCallback(this,
                                               &OladHTTPServer::HandleGetDmx,
                                               response));

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);

  return MHD_YES;
}


/*
 * Handle the set dmx command
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::HandleSetDmx(const HTTPRequest *request,
                                HTTPResponse *response) {
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
      NewSingleCallback(this, &OladHTTPServer::HandleBoolResponse, response));

  if (!ok)
    return m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/*
 * Cause the server to shutdown
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::DisplayQuit(const HTTPRequest *request,
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
  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  int r = response->Send();
  delete response;
  return r;
  (void) request;
}


/*
 * Reload all plugins
 * @param request the HTTPRequest
 * @param response the HTTPResponse
 * @returns MHD_NO or MHD_YES
 */
int OladHTTPServer::ReloadPlugins(const HTTPRequest *request,
                                 HTTPResponse *response) {
  m_ola_server->ReloadPlugins();
  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->Append("ok");
  int r = response->Send();
  delete response;
  return r;
  (void) request;
}


/*
 * Handle the plugin list callback
 * @param response the HTTPResponse that is associated with the request.
 * @param plugins a list of plugins
 * @param error an error string.
 */
void OladHTTPServer::HandlePluginList(HTTPResponse *response,
                                     const vector<OlaPlugin> &plugins,
                                     const string &error) {
  if (!error.empty()) {
    m_server.ServeError(response, error);
    return;
  }

  JsonObject *json = new JsonObject();

  // fire off the universe request now. the main server is running in a
  // separate thread.
  bool ok = m_client.FetchUniverseList(
      NewSingleCallback(this,
                        &OladHTTPServer::HandleUniverseList,
                        response,
                        json));

  if (!ok) {
    m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
    delete json;
    return;
  }

  JsonArray *plugins_json = json->AddArray("plugins");
  vector<OlaPlugin>::const_iterator iter;
  for (iter = plugins.begin(); iter != plugins.end(); ++iter) {
    JsonObject *plugin = plugins_json->AppendObject();
    plugin->Add("name", iter->Name());
    plugin->Add("id", iter->Id());
  }
}


/*
 * Handle the universe list callback
 * @param response the HTTPResponse that is associated with the request.
 * @param plugins a list of plugins
 * @param error an error string.
 */
void OladHTTPServer::HandleUniverseList(HTTPResponse *response,
                                       JsonObject *json,
                                       const vector<OlaUniverse> &universes,
                                       const string &error) {
  if (error.empty()) {
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

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(*json);
  delete response;
  delete json;
}


/*
 * Handle the plugin description response.
 * @param response the HTTPResponse that is associated with the request.
 * @param description the plugin description.
 * @param error an error string.
 */
void OladHTTPServer::HandlePluginInfo(HTTPResponse *response,
                                     const string &description,
                                     const string &error) {
  if (!error.empty()) {
    m_server.ServeError(response, error);
    return;
  }
  string escaped_description = description;
  Escape(&escaped_description);

  JsonObject json;
  json.Add("description", description);

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete response;
}


/*
 * Handle the universe info
 * @param response the HTTPResponse that is associated with the request.
 * @param universe the OlaUniverse object
 * @param error an error string.
 */
void OladHTTPServer::HandleUniverseInfo(HTTPResponse *response,
                                       OlaUniverse &universe,
                                       const string &error) {
  if (!error.empty()) {
    m_server.ServeError(response, error);
    return;
  }

  JsonObject *json = new JsonObject();

  // fire off the device/port request now. the main server is running in a
  // separate thread.
  bool ok = m_client.FetchDeviceInfo(
      ola::OLA_PLUGIN_ALL,
      NewSingleCallback(this,
                        &OladHTTPServer::HandlePortsForUniverse,
                        response,
                        json,
                        universe.Id()));

  if (!ok) {
    m_server.ServeError(response, K_BACKEND_DISCONNECTED_ERROR);
    delete json;
    return;
  }

  json->Add("id", universe.Id());
  json->Add("name", universe.Name());
  json->Add("merge_mode",
           (universe.MergeMode() == OlaUniverse::MERGE_HTP ? "HTP" : "LTP"));
}


void OladHTTPServer::HandlePortsForUniverse(
    HTTPResponse *response,
    JsonObject *json,
    unsigned int universe_id,
    const vector<OlaDevice> &devices,
    const string &error) {
  if (error.empty()) {
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

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(*json);
  delete json;
  delete response;
}


/*
 * Handle the list of candidate ports
 * @param response the HTTPResponse that is associated with the request.
 * @param devices the possbile devices & ports
 * @param error an error string.
 */
void OladHTTPServer::HandleCandidatePorts(
    HTTPResponse *response,
    const vector<class OlaDevice> &devices,
    const string &error) {
  if (!error.empty()) {
    m_server.ServeError(response, error);
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

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete response;
}


/*
 * Schedule a callback to send the new universe response to the client
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
 * Send the response to a new universe request
 */
void OladHTTPServer::SendCreateUniverseResponse(
    HTTPResponse *response,
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

  JsonObject json;
  json.Add("ok", !failed);
  json.Add("universe", universe_id);
  json.Add("message", (failed ? "Failed to patch any ports" : ""));

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete action_queue;
  delete response;
}


/*
 * Schedule a callback to send the modify universe response to the client
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
 * Send the response to a modify universe request.
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


/*
 * Callback for m_client.FetchDmx called by GetDmx
 * @param response the HTTPResponse
 * @param buffer the DmxBuffer
 * @param error Error message
 */
void OladHTTPServer::HandleGetDmx(HTTPResponse *response,
                                 const DmxBuffer &buffer,
                                 const string &error) {
  // rather than adding 512 JsonValue we cheat and use raw here
  stringstream str;
  str << "[" << buffer.ToString() << "]";
  JsonObject json;
  json.AddRaw("dmx", str.str());
  json.Add("error", error);

  response->SetHeader("Cache-Control", "no-cache, must-revalidate");
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->SendJson(json);
  delete response;
}


/*
 * Handle the set DMX response.
 * @param response the HTTPResponse that is associated with the request.
 * @param error an error string.
 */
void OladHTTPServer::HandleBoolResponse(HTTPResponse *response,
                                       const string &error) {
  if (!error.empty()) {
    m_server.ServeError(response, error);
    return;
  }
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);
  response->Append("ok");
  response->Send();
  delete response;
}


/**
 * Add the json representation of this port to the stringstream
 */
void OladHTTPServer::PortToJson(JsonObject *json,
                               const OlaDevice &device,
                               const OlaPort &port,
                               bool is_output) {
  stringstream str;
  str << device.Alias() << "-" << (is_output ? "O" : "I") << "-" << port.Id();

  json->Add("device", device.Name());
  json->Add("description", port.Description());
  json->Add("id", str.str());
  json->Add("is_output", is_output);

  if (port.PriorityCapability() != CAPABILITY_NONE) {
    JsonObject *priority_json = json->AddObject("priority");
    priority_json->Add("value", static_cast<int>(port.Priority()));
    priority_json->Add(
      "current_mode",
      (port.PriorityMode() == PRIORITY_MODE_INHERIT ?  "inherit" : "override"));
  }
}


/*
 * Add the Patch Actions to the ActionQueue.
 * @param action_queue the ActionQueue to add the actions to.
 * @param port_id_string a string to ports to add/remove.
 * @param universe the universe id to add these ports if
 * @param port_action either PATCH or UNPATCH.
 */
void OladHTTPServer::AddPatchActions(ActionQueue *action_queue,
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
void OladHTTPServer::DecodePortIds(const string &port_ids,
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


/*
 * Register a handler
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
}  // ola
