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
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>

#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/StringUtils.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "olad/Device.h"
#include "olad/DeviceManager.h"
#include "olad/OlaHttpServer.h"
#include "olad/OlaServer.h"
#include "olad/OlaVersion.h"
#include "olad/Plugin.h"
#include "olad/PluginManager.h"
#include "olad/Port.h"
#include "olad/Universe.h"
#include "olad/UniverseStore.h"

#include "ola/Logging.h"

namespace ola {

using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::vector;

const char OlaHttpServer::K_DATA_DIR_VAR[] = "http_data_dir";
const char OlaHttpServer::K_UPTIME_VAR[] = "uptime-in-ms";


OlaHttpServer::OlaHttpServer(ExportMap *export_map,
                             SelectServer *ss,
                             OlaServer *ola_server,
                             UniverseStore *universe_store,
                             PluginManager *plugin_manager,
                             DeviceManager *device_manager,
                             PortManager *port_manager,
                             unsigned int port,
                             bool enable_quit,
                             const string &data_dir,
                             const ola::network::Interface &interface)
    : m_server(port, data_dir),
      m_export_map(export_map),
      m_ss(ss),
      m_ola_server(ola_server),
      m_universe_store(universe_store),
      m_plugin_manager(plugin_manager),
      m_device_manager(device_manager),
      m_port_manager(port_manager),
      m_enable_quit(enable_quit),
      m_interface(interface) {

  // The main handlers
  RegisterHandler("/", &OlaHttpServer::DisplayIndex);
  RegisterHandler("/debug", &OlaHttpServer::DisplayDebug);
  RegisterHandler("/help", &OlaHttpServer::DisplayHandlers);
  RegisterHandler("/quit", &OlaHttpServer::DisplayQuit);
  RegisterHandler("/reload", &OlaHttpServer::ReloadPlugins);
  RegisterHandler("/run_rdm_discovery", &OlaHttpServer::RunRDMDiscovery);
  RegisterHandler("/new_universe", &OlaHttpServer::CreateNewUniverse);
  RegisterHandler("/modify_universe", &OlaHttpServer::ModifyUniverse);
  RegisterHandler("/set_dmx", &OlaHttpServer::HandleSetDmx);

  // json endpoints for the new UI
  RegisterHandler("/json/server_stats", &OlaHttpServer::JsonServerStats);
  RegisterHandler("/json/universe_plugin_list",
                  &OlaHttpServer::JsonUniversePluginList);
  RegisterHandler("/json/plugin_info", &OlaHttpServer::JsonPluginInfo);
  RegisterHandler("/json/get_ports", &OlaHttpServer::JsonAvailablePorts);
  RegisterHandler("/json/universe_info", &OlaHttpServer::JsonUniverseInfo);
  RegisterHandler("/json/uids", &OlaHttpServer::JsonUIDs);

  // these are the static files for the new UI
  RegisterFile("blank.gif", HttpServer::CONTENT_TYPE_GIF);
  RegisterFile("button-bg.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("custombutton.css", HttpServer::CONTENT_TYPE_CSS);
  RegisterFile("expander.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("handle.vertical.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("loader.gif", HttpServer::CONTENT_TYPE_GIF);
  RegisterFile("logo.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("ola.html", HttpServer::CONTENT_TYPE_HTML);
  RegisterFile("ola.js", HttpServer::CONTENT_TYPE_JS);
  RegisterFile("tick.gif", HttpServer::CONTENT_TYPE_GIF);
  RegisterFile("toolbar-bg.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("toolbar.css", HttpServer::CONTENT_TYPE_CSS);
  RegisterFile("vertical.gif", HttpServer::CONTENT_TYPE_GIF);

  StringVariable *data_dir_var = export_map->GetStringVar(K_DATA_DIR_VAR);
  data_dir_var->Set(m_server.DataDir());
  Clock::CurrentTime(&m_start_time);
  m_start_time_t = time(NULL);
  export_map->GetStringVar(K_UPTIME_VAR);
}


/**
 * Stop the HttpServer and wait for it to exit.
 */
void OlaHttpServer::Stop() {
  OLA_INFO << "Notifying HTTP server thread to stop";
  m_server.Stop();
  OLA_INFO << "Waiting for HTTP server thread to exit";
  m_server.Join();
  OLA_INFO << "HTTP server thread exited";
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
  str << "  \"ip\": \"" <<
    ola::network::AddressToString(m_interface.ip_address) << "\"," << endl;
  str << "  \"version\": \"" << OLA_VERSION << "\"," << endl;
  str << "  \"up_since\": \"" << start_time_str << "\"," << endl;
  str << "  \"quit_enabled\": " << m_enable_quit << "," << endl;
  str << "}";

  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  return response->Send();
  (void) request;
}


/**
 * Print the list of universes / plugins as a json string
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::JsonUniversePluginList(const HttpRequest *request,
                                          HttpResponse *response) {
  stringstream str;
  str << "{" << endl;
  str << "  \"plugins\": [" << endl;

  vector<AbstractPlugin*> plugins;
  vector<AbstractPlugin*>::const_iterator iter;
  m_plugin_manager->Plugins(&plugins);
  std::sort(plugins.begin(), plugins.end(), PluginLessThan());

  for (iter = plugins.begin(); iter != plugins.end(); ++iter) {
    str << "    {\"name\": \"" << EscapeString((*iter)->Name()) <<
      "\", \"id\": " << (*iter)->Id() << "}," << endl;
  }

  str << "  ]," << endl;
  str << "  \"universes\": [" << endl;

  vector<Universe*> universes;
  vector<Universe*>::const_iterator universe_iter;
  m_universe_store->GetList(&universes);
  for (universe_iter = universes.begin(); universe_iter != universes.end();
       ++universe_iter) {
    str << "    {" << endl;
    str << "      \"id\": " << (*universe_iter)->UniverseId() << "," << endl;
    str << "      \"input_ports\": " << (*universe_iter)->InputPortCount() <<
      "," << endl;
    str << "      \"name\": \"" << EscapeString((*universe_iter)->Name()) <<
      "\"," << endl;
    str << "      \"output_ports\": " << (*universe_iter)->OutputPortCount() <<
      "," << endl;
    str << "      \"rdm_devices\": " << (*universe_iter)->UIDCount() << ","
       << endl;
    str << "    }," << endl;
  }

  str << "  ]," << endl;
  str << "}";

  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  return response->Send();
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
  AbstractPlugin *plugin = NULL;
  plugin = m_plugin_manager->GetPlugin((ola_plugin_id) plugin_id);

  if (!plugin)
    return m_server.ServeNotFound(response);

  string description = plugin->Description();
  Escape(&description);
  stringstream str;
  str << "{\"description\": \"" << description;
  str << "\"}";

  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  return response->Send();
  (void) request;
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
  if (!StringToUInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  Universe *universe = m_universe_store->GetUniverse(universe_id);

  if (!universe)
    return m_server.ServeNotFound(response);

  stringstream str;
  str << "{" << endl;
  str << "  \"id\": " << universe->UniverseId() << "," << endl;
  str << "  \"name\": \"" << EscapeString(universe->Name()) << "\"," << endl;
  str << "  \"merge_mode\": \"" <<
    (universe->MergeMode() == Universe::MERGE_HTP ? "HTP" : "LTP") << "\"," <<
    endl;

  vector<InputPort*> input_ports;
  universe->InputPorts(&input_ports);
  vector<InputPort*>::const_iterator input_iter = input_ports.begin();
  str << "  \"input_ports\": [" << endl;
  for (; input_iter != input_ports.end(); input_iter++) {
    PortToJson(*input_iter, &str);
  }
  str << "  ]," << endl;

  vector<OutputPort*> output_ports;
  universe->OutputPorts(&output_ports);
  vector<OutputPort*>::const_iterator output_iter = output_ports.begin();
  str << "  \"output_ports\": [" << endl;
  for (; output_iter != output_ports.end(); output_iter++) {
    PortToJson(*output_iter, &str);
  }
  str << "  ]," << endl;
  str << "}" << endl;

  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  return response->Send();
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
  unsigned int universe_id;
  Universe *universe = NULL;
  if ((!uni_id.empty()) && StringToUInt(uni_id, &universe_id))
    universe = m_universe_store->GetUniverse(universe_id);

  stringstream str;
  str << "[" << endl;

  vector<InputPort*> input_ports;
  vector<OutputPort*> output_ports;

  vector<device_alias_pair> device_pairs = m_device_manager->Devices();
  sort(device_pairs.begin(), device_pairs.end());
  vector<device_alias_pair>::const_iterator iter;

  for (iter = device_pairs.begin(); iter != device_pairs.end(); ++iter) {
    AbstractDevice *device = iter->device;
    input_ports.clear();
    output_ports.clear();
    device->InputPorts(&input_ports);
    device->OutputPorts(&output_ports);

    vector<InputPort*>::const_iterator input_iter;
    vector<OutputPort*>::const_iterator output_iter;
    // check if this device already has ports bound to this universe
    bool seen_input_port = false;
    bool seen_output_port = false;
    for (input_iter = input_ports.begin(); input_iter != input_ports.end();
         input_iter++) {
      if (universe && (*input_iter)->GetUniverse() == universe)
        seen_input_port = true;
    }

    for (output_iter = output_ports.begin();
         output_iter != output_ports.end(); output_iter++) {
      if (universe && (*output_iter)->GetUniverse() == universe)
        seen_output_port = true;
    }

    for (input_iter = input_ports.begin(); input_iter != input_ports.end();
         input_iter++) {
      if ((*input_iter)->GetUniverse())
        continue;
      if (seen_output_port && !device->AllowLooping())
        continue;
      if (seen_input_port && !device->AllowMultiPortPatching())
        break;

      str << "  {" << endl;
      str << "    \"device\": \"" << EscapeString(device->Name()) << "\","
        << endl;
      str << "    \"id\": \"" << (*input_iter)->UniqueId() << "\"," << endl;
      str << "    \"is_output\": false," << endl;
      str << "    \"description\": \"" <<
        EscapeString((*input_iter)->Description()) << "\"," << endl;
      str << "  }," << endl;

      if (!device->AllowMultiPortPatching())
        break;
    }

    for (output_iter = output_ports.begin();
         output_iter != output_ports.end(); output_iter++) {
      if ((*output_iter)->GetUniverse())
        continue;
      if (seen_input_port && !device->AllowLooping())
        continue;
      if (seen_output_port && !device->AllowMultiPortPatching())
        break;

      str << "  {" << endl;
      str << "    \"device\": \"" << EscapeString(device->Name()) << "\","
        << endl;
      str << "    \"id\": \"" << (*output_iter)->UniqueId() << "\"," << endl;
      str << "    \"is_output\": true," << endl;
      str << "    \"description\": \"" <<
        EscapeString((*output_iter)->Description()) << "\"," << endl;
      str << "  }," << endl;

      if (!device->AllowMultiPortPatching())
        break;
    }
  }
  str << "]" << endl;

  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  return response->Send();
  (void) request;
}


/**
 * Return the list of uids for this universe as json
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::JsonUIDs(const HttpRequest *request,
                            HttpResponse *response) {
  string uni_id = request->GetParameter("id");
  unsigned int universe_id;
  if (!StringToUInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  Universe *universe = m_universe_store->GetUniverse(universe_id);

  if (!universe)
    return m_server.ServeNotFound(response);

  ola::rdm::UIDSet uids;
  universe->GetUIDs(&uids);
  ola::rdm::UIDSet::Iterator iter = uids.Begin();

  stringstream str;
  str << "{" << endl;
  str << "  \"universe\": " << universe->UniverseId() << "," << endl;
  str << "  \"uids\": [" << endl;

  for (; iter != uids.End(); ++iter) {
    str << "    {" << endl;
    str << "       \"manufacturer_id\": " << iter->ManufacturerId() << ","
      << endl;
    str << "       \"device_id\": " << iter->DeviceId() << "," << endl;
    str << "    }," << endl;
  }

  str << "  ]" << endl;
  str << "}";

  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  return response->Send();
  (void) request;
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

  unsigned int universe_id;
  if (!StringToUInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  bool status = true;
  string message;
  Universe *universe = m_universe_store->GetUniverse(universe_id);

  if (universe) {
    status = false;
    message = "Universe already exists";
  } else {
    UpdatePortsForUniverse(universe_id, request);
    universe = m_universe_store->GetUniverse(universe_id);

    if (universe) {
      string name = request->GetPostParameter("name");
      if (!name.empty())
        universe->SetName(name);
    } else {
      status = false;
      message = "No ports patched.";
    }
  }

  stringstream str;
  str << "{" << endl;
  str << "  \"ok\": " << status << "," << endl;
  str << "  \"universe\": " << universe_id << "," << endl;
  str << "  \"message\": \"" << EscapeString(message) << "\"," << endl;
  str << "}";

  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  return response->Send();
  (void) request;
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
  string add_ports = request->GetPostParameter("add_ports");
  string remove_ports = request->GetPostParameter("remove_ports");

  unsigned int universe_id;
  if (!StringToUInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  Universe *universe = m_universe_store->GetUniverse(universe_id);
  if (!universe)
    return m_server.ServeNotFound(response);

  if (name.size() > K_UNIVERSE_NAME_LIMIT)
    name = name.substr(K_UNIVERSE_NAME_LIMIT);

  if (!name.empty())
    universe->SetName(name);

  if (merge_mode == "LTP")
    universe->SetMergeMode(Universe::MERGE_LTP);
  else
    universe->SetMergeMode(Universe::MERGE_HTP);

  string message;
  UpdatePortsForUniverse(universe_id, request);

  stringstream str;
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  return response->Send();
  (void) request;
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
  file_info.file_path = "ola.html";
  file_info.content_type = HttpServer::CONTENT_TYPE_HTML;
  return m_server.ServeStaticContent(&file_info, response);
  (void) request;
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
  if (!StringToUInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  Universe *universe = m_universe_store->GetUniverse(universe_id);
  if (!universe)
    return m_server.ServeNotFound(response);

  DmxBuffer buffer;
  buffer.SetFromString(dmx_data_str);
  if (buffer.Size())
    universe->SetDMX(buffer);
  response->Append("ok");
  return response->Send();
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
  Clock::CurrentTime(&now);
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
  return response->Send();
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
    m_ss->Terminate();
  } else {
    response->SetStatus(403);
    response->SetContentType(HttpServer::CONTENT_TYPE_HTML);
    response->Append("<b>403 Unauthorized</b>");
  }
  return response->Send();
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
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append("ok");
  return response->Send();
  (void) request;
}


/**
 * Run RDM discovery for a universe
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::RunRDMDiscovery(const HttpRequest *request,
                                   HttpResponse *response) {
  string uni_id = request->GetParameter("id");
  unsigned int universe_id;
  if (!StringToUInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  Universe *universe = m_universe_store->GetUniverse(universe_id);

  if (!universe)
    return m_server.ServeNotFound(response);

  universe->RunRDMDiscovery();
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append("ok");
  return response->Send();
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
  return response->Send();
  (void) request;
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


/*
 * Update the port priorities from the data in a HTTP request.
 */
template <class PortClass>
void OlaHttpServer::UpdatePortPriorites(const HttpRequest *request,
                                        vector<PortClass*> *ports) {
  typename vector<PortClass*>::iterator iter = ports->begin();

  for (;iter != ports->end(); ++iter) {
    if ((*iter)->PriorityCapability() == CAPABILITY_NONE)
      continue;

    string priority_mode_id = (*iter)->UniqueId() +
      DeviceManager::PRIORITY_MODE_SUFFIX;
    string priority_id = (*iter)->UniqueId() +
      DeviceManager::PRIORITY_VALUE_SUFFIX;

    string value = request->GetPostParameter(priority_id);
    string mode = request->GetPostParameter(priority_mode_id);

    if (!value.empty() || !mode.empty())
      m_port_manager->SetPriority(*iter, mode, value);
  }
}


/**
 * Add the json representation of this port to the stringstream
 */
void OlaHttpServer::PortToJson(const Port *port, stringstream *str) {
  // skip ports which don't have parent devices like the InternalInputPort
  if (!port->GetDevice())
    return;

  *str << "    {" << endl;
  *str << "      \"device\": \"" << EscapeString(port->GetDevice()->Name())
    << "\"," << endl;
  *str << "      \"description\": \"" <<
    EscapeString(port->Description()) << "\"," << endl;
  *str << "      \"id\": \"" << port->UniqueId() << "\"," << endl;

  if (port->PriorityCapability() != CAPABILITY_NONE) {
    *str << "      \"priority\": {" << endl;
    *str << "        \"value\": " << static_cast<int>(port->GetPriority()) <<
      "," << endl;
    if (port->PriorityCapability() == CAPABILITY_FULL) {
      *str << "        \"current_mode\": \"" <<
        (port->GetPriorityMode() == PRIORITY_MODE_INHERIT ?
         "inherit" : "override") << "\"," <<
        endl;
    }
    *str << "      }" << endl;
  }
  *str << "    }," << endl;
}


/**
 * Add a list of ports to a universe and set the name
 */
bool OlaHttpServer::UpdatePortsForUniverse(unsigned int universe_id,
                                           const HttpRequest *request) {
  string add_port_ids = request->GetPostParameter("add_ports");
  string remove_port_ids = request->GetPostParameter("remove_ports");

  vector<string> add_ports;
  vector<string> remove_ports;
  StringSplit(add_port_ids, add_ports, ",");
  StringSplit(remove_port_ids, remove_ports, ",");

  vector<InputPort*> input_ports;
  vector<OutputPort*> output_ports;

  // There is no way to look up a port based on id, so we need to loop through
  // all of them for now.
  vector<device_alias_pair> device_pairs = m_device_manager->Devices();
  vector<device_alias_pair>::const_iterator iter;
  for (iter = device_pairs.begin(); iter != device_pairs.end(); ++iter) {
    input_ports.clear();
    output_ports.clear();
    iter->device->InputPorts(&input_ports);
    iter->device->OutputPorts(&output_ports);

    vector<InputPort*>::iterator input_iter = input_ports.begin();
    for (; input_iter != input_ports.end(); input_iter++) {
      UpdatePortForUniverse(universe_id, *input_iter, add_ports, remove_ports);
    }

    vector<OutputPort*>::iterator output_iter = output_ports.begin();
    for (; output_iter != output_ports.end(); output_iter++) {
      UpdatePortForUniverse(universe_id, *output_iter, add_ports, remove_ports);
    }

    UpdatePortPriorites(request, &input_ports);
    UpdatePortPriorites(request, &output_ports);
  }
  return true;
}


/*
 * Update a single port from the http request
 */
template <class PortClass>
void OlaHttpServer::UpdatePortForUniverse(unsigned int universe_id,
                                          PortClass *port,
                                          const vector<string> &ids_to_add,
                                          const vector<string> &ids_to_remove) {
  vector<string>::const_iterator iter;
  for (iter = ids_to_add.begin(); iter != ids_to_add.end(); ++iter) {
    if (port->UniqueId() == *iter) {
      m_port_manager->PatchPort(port, universe_id);
    }
  }
  for (iter = ids_to_remove.begin(); iter != ids_to_remove.end(); ++iter) {
    if (port->UniqueId() == *iter) {
      m_port_manager->UnPatchPort(port);
    }
  }
}
}  // ola
