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

#include "ola/DmxBuffer.h"
#include "ola/StringUtils.h"
#include "ola/network/NetworkUtils.h"
#include "olad/Device.h"
#include "olad/DeviceManager.h"
#include "olad/OlaHttpServer.h"
#include "olad/Plugin.h"
#include "olad/PluginManager.h"
#include "olad/Port.h"
#include "olad/OlaServer.h"
#include "olad/Universe.h"
#include "olad/UniverseStore.h"

#include "ola/Logging.h"

namespace ola {

using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::vector;
using ctemplate::TemplateDictionary;
using ctemplate::TemplateNamelist;

const char OlaHttpServer::K_DATA_DIR_VAR[] = "http_data_dir";
const char OlaHttpServer::K_UPTIME_VAR[] = "uptime-in-ms";

RegisterTemplateFilename(MAIN_FILENAME, "show_main_page.tpl");
RegisterTemplateFilename(PLUGINS_FILENAME, "show_loaded_plugins.tpl");
RegisterTemplateFilename(PLUGIN_INFO_FILENAME, "show_plugin_info.tpl");
RegisterTemplateFilename(DEVICE_FILENAME, "show_loaded_devices.tpl");
RegisterTemplateFilename(UNIVERSE_FILENAME, "show_universe_settings.tpl");
RegisterTemplateFilename(CONSOLE_FILENAME, "show_dmx_console.tpl");

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
  RegisterHandler("/debug", &OlaHttpServer::DisplayDebug);
  RegisterHandler("/quit", &OlaHttpServer::DisplayQuit);
  RegisterHandler("/reload", &OlaHttpServer::ReloadPlugins);
  RegisterHandler("/help", &OlaHttpServer::DisplayHandlers);
  RegisterHandler("/main", &OlaHttpServer::DisplayMain);
  RegisterHandler("/plugins", &OlaHttpServer::DisplayPlugins);
  RegisterHandler("/plugin", &OlaHttpServer::DisplayPluginInfo);
  RegisterHandler("/devices", &OlaHttpServer::DisplayDevices);
  RegisterHandler("/universes", &OlaHttpServer::DisplayUniverses);
  RegisterHandler("/console", &OlaHttpServer::DisplayConsole);
  RegisterHandler("/reload_templates", &OlaHttpServer::DisplayTemplateReload);
  RegisterHandler("/set_dmx", &OlaHttpServer::HandleSetDmx);
  RegisterHandler("/", &OlaHttpServer::DisplayIndex);
  RegisterFile("index.html", HttpServer::CONTENT_TYPE_HTML);
  RegisterFile("menu.html", HttpServer::CONTENT_TYPE_HTML);
  RegisterFile("about.html", HttpServer::CONTENT_TYPE_HTML);
  RegisterFile("console_values.html", HttpServer::CONTENT_TYPE_HTML);
  RegisterFile("simple.css", HttpServer::CONTENT_TYPE_CSS);
  RegisterFile("bluecurve.css", HttpServer::CONTENT_TYPE_CSS);
  RegisterFile("notice.gif", HttpServer::CONTENT_TYPE_GIF);
  RegisterFile("plus.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("forward.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("back.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("full.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("dbo.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("save.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("load.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("minus.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("handle.vertical.png", HttpServer::CONTENT_TYPE_PNG);
  RegisterFile("ajax_request.js", HttpServer::CONTENT_TYPE_JS);
  RegisterFile("console.js", HttpServer::CONTENT_TYPE_JS);
  RegisterFile("range.js", HttpServer::CONTENT_TYPE_JS);
  RegisterFile("slider.js", HttpServer::CONTENT_TYPE_JS);
  RegisterFile("timer.js", HttpServer::CONTENT_TYPE_JS);
  RegisterFile("GPL.txt", HttpServer::CONTENT_TYPE_PLAIN);
  m_server.RegisterFile("/boxsizing.htc", "boxsizing.htc", "text/x-component");

  StringVariable *data_dir_var = export_map->GetStringVar(K_DATA_DIR_VAR);
  data_dir_var->Set(m_server.DataDir());
  Clock::CurrentTime(&m_start_time);
  export_map->GetStringVar(K_UPTIME_VAR);

  // warn on any missing templates
  TemplateNamelist::GetMissingList(false);
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
  file_info.file_path = "index.html";
  file_info.content_type = HttpServer::CONTENT_TYPE_HTML;
  return m_server.ServeStaticContent(&file_info, response);
  (void) request;
}


/*
 * Display the main page
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::DisplayMain(const HttpRequest *request,
                               HttpResponse *response) {
  TemplateDictionary dict("main");
  TimeStamp now;
  Clock::CurrentTime(&now);
  TimeInterval diff = now - m_start_time;

  stringstream str;
  unsigned int minutes = diff.Seconds() / 60;
  unsigned int hours = minutes / 60;
  str << hours << " hours, " << minutes % 60 << " minutes, " <<
    diff.Seconds() % 60 << " seconds";
  dict.SetValue("UPTIME", str.str());
  dict.SetValue("HOSTNAME", ola::network::FullHostname());
  dict.SetValue("IP", ola::network::AddressToString(m_interface.ip_address));

  if (m_enable_quit)
    dict.ShowSection("QUIT_ENABLED");
  return m_server.DisplayTemplate(MAIN_FILENAME, &dict, response);
  (void) request;
}



/*
 * Display the plugins page.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::DisplayPlugins(const HttpRequest *request,
                                  HttpResponse *response) {
  TemplateDictionary dict("plugins");
  vector<AbstractPlugin*> plugins;
  m_plugin_manager->Plugins(&plugins);
  std::sort(plugins.begin(), plugins.end(), PluginLessThan());

  if (plugins.size()) {
    int i = 1;
    vector<AbstractPlugin*>::const_iterator iter;
    for (iter = plugins.begin(); iter != plugins.end(); ++iter) {
      TemplateDictionary *sub_dict = dict.AddSectionDictionary("PLUGIN");
      sub_dict->SetValue("ID", IntToString((*iter)->Id()));
      sub_dict->SetValue("NAME", (*iter)->Name());
      if (i % 2)
        sub_dict->ShowSection("ODD");
      i++;
    }
  } else {
    dict.ShowSection("NO_PLUGINS");
  }
  return m_server.DisplayTemplate(PLUGINS_FILENAME, &dict, response);
  (void) request;
}


/*
 * Display the info for a single plugin.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::DisplayPluginInfo(const HttpRequest *request,
                                     HttpResponse *response) {
  string val = request->GetParameter("id");
  int plugin_id = atoi(val.data());
  AbstractPlugin *plugin = NULL;
  plugin = m_plugin_manager->GetPlugin((ola_plugin_id) plugin_id);

  if (!plugin)
    return m_server.ServeNotFound(response);

  TemplateDictionary dict("plugin");
  dict.SetValue("NAME", plugin->Name());
  dict.SetValue("DESCRIPTION", plugin->Description());
  return m_server.DisplayTemplate(PLUGIN_INFO_FILENAME, &dict, response);
}


/*
 * Display the device page.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::DisplayDevices(const HttpRequest *request,
                                  HttpResponse *response) {
  TemplateDictionary dict("device");
  vector<device_alias_pair> device_pairs = m_device_manager->Devices();

  string action = request->GetPostParameter("action");
  bool save_changes = !action.empty();

  if (device_pairs.size()) {
    vector<device_alias_pair>::const_iterator iter;
    sort(device_pairs.begin(), device_pairs.end());
    for (iter = device_pairs.begin(); iter != device_pairs.end(); ++iter) {
      TemplateDictionary *sub_dict = dict.AddSectionDictionary("DEVICE");
      PopulateDeviceDict(request, sub_dict, *iter, save_changes);
    }
  } else {
    dict.ShowSection("NO_DEVICES");
  }
  return m_server.DisplayTemplate(DEVICE_FILENAME, &dict, response);
}


/*
 * Show the universe settings page.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::DisplayUniverses(const HttpRequest *request,
                                    HttpResponse *response) {
  TemplateDictionary dict("universes");
  vector<Universe*> universes;
  m_universe_store->GetList(&universes);

  string action = request->GetParameter("action");
  bool save_changes = !action.empty();

  if (universes.size()) {
    vector<Universe*>::const_iterator iter;
    int i = 1;
    for (iter = universes.begin(); iter != universes.end(); ++iter) {
      if (save_changes) {
        string uni_name = request->GetParameter(
            "name_" + IntToString((*iter)->UniverseId()));
        string uni_mode = request->GetParameter(
            "mode_" + IntToString((*iter)->UniverseId()));

        if (uni_name.size() > K_UNIVERSE_NAME_LIMIT)
          uni_name = uni_name.substr(K_UNIVERSE_NAME_LIMIT);
        (*iter)->SetName(uni_name);

        if (uni_mode == "ltp")
          (*iter)->SetMergeMode(Universe::MERGE_LTP);
        else
          (*iter)->SetMergeMode(Universe::MERGE_HTP);
      }
      TemplateDictionary *sub_dict = dict.AddSectionDictionary("UNIVERSE");
      sub_dict->SetValue("ID", IntToString((*iter)->UniverseId()));
      sub_dict->SetValue("NAME", (*iter)->Name());
      if ((*iter)->MergeMode() == Universe::MERGE_HTP)
        sub_dict->ShowSection("HTP_MODE");
      if (i % 2)
        sub_dict->ShowSection("ODD");
      i++;
    }
  } else {
    dict.ShowSection("NO_UNIVERSES");
  }
  return m_server.DisplayTemplate(UNIVERSE_FILENAME, &dict, response);
}


/*
 * Show the Dmx console page.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int OlaHttpServer::DisplayConsole(const HttpRequest *request,
                                  HttpResponse *response) {
  string uni_id = request->GetParameter("u");
  unsigned int universe_id;
  if (!StringToUInt(uni_id, &universe_id))
    return m_server.ServeNotFound(response);

  Universe *universe = m_universe_store->GetUniverse(universe_id);

  if (!universe)
    return m_server.ServeNotFound(response);

  TemplateDictionary dict("console");
  dict.SetValue("ID", IntToString(universe->UniverseId()));
  dict.SetValue("NAME", universe->Name());

  for (unsigned int i = 0; i <= K_CONSOLE_SLIDERS; i++) {
    TemplateDictionary *sliders_dict = dict.AddSectionDictionary("SLIDERS");
    sliders_dict->SetValue("INDEX", IntToString(i));
  }

  return m_server.DisplayTemplate(CONSOLE_FILENAME, &dict, response);
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


/*
 * Handle the template reload.
 */
int OlaHttpServer::DisplayTemplateReload(const HttpRequest *request,
                                         HttpResponse *response) {
  ctemplate::Template::ReloadAllIfChanged();
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
  m_server.RegisterHandler(path, NewHttpClosure(this, method));
}


/*
 * Register a static file
 */
inline void OlaHttpServer::RegisterFile(const string &file,
                                 const string &content_type) {
  m_server.RegisterFile("/" + file, file, content_type);
}

/*
 * Populate a dictionary for this device.
 * @param dict the dictionary to fill
 * @param device the device to use
 */
void OlaHttpServer::PopulateDeviceDict(const HttpRequest *request,
                                       TemplateDictionary *dict,
                                       const device_alias_pair &device_pair,
                                       bool save_changes) {
  AbstractDevice *device = device_pair.device;
  dict->SetValue("ID", IntToString(device_pair.alias));
  dict->SetValue("NAME", device->Name());
  string val = request->GetPostParameter("show_" +
                                         IntToString(device_pair.alias));
  dict->SetValue("SHOW_VALUE", val == "1" ? "1" : "0");
  dict->SetValue("SHOW", val == "1" ? "block" : "none");

  vector<InputPort*> input_ports;
  device->InputPorts(&input_ports);
  vector<OutputPort*> output_ports;
  device->OutputPorts(&output_ports);

  if (save_changes) {
    UpdatePortPatchings(request, &input_ports);
    UpdatePortPatchings(request, &output_ports);
    UpdatePortPriorites(request, &input_ports);
    UpdatePortPriorites(request, &output_ports);
  }

  unsigned int offset = 0;
  AddPortsToDict(dict, input_ports, &offset);
  AddPortsToDict(dict, output_ports, &offset);
}


/*
 * Update the port patchings from the data in a HTTP request.
 */
template <class PortClass>
void OlaHttpServer::UpdatePortPatchings(const HttpRequest *request,
                                        vector<PortClass*> *ports) {
  typename vector<PortClass*>::iterator iter = ports->begin();

  while (iter != ports->end()) {
    string port_id = (*iter)->UniqueId();
    string uni_id = request->GetPostParameter(port_id);
    unsigned int universe_id;

    if (StringToUInt(uni_id, &universe_id))
      // valid universe number, patch this universe
      m_port_manager->PatchPort(*iter, universe_id);
    else
      m_port_manager->UnPatchPort(*iter);
    iter++;
  }
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

    m_port_manager->SetPriority(
        *iter,
        request->GetPostParameter(priority_mode_id),
        request->GetPostParameter(priority_id));
  }
}


/*
 * Fill in a template dictionary with a list of ports.
 * @param dict the dictionary to fill
 * @param ports the vector of ports
 */
template <class PortClass>
void OlaHttpServer::AddPortsToDict(TemplateDictionary *dict,
                                   const vector<PortClass*> &ports,
                                   unsigned int *offset) {
  typename vector<PortClass*>::const_iterator iter = ports.begin();

  while (iter != ports.end()) {
    TemplateDictionary *port_dict = dict->AddSectionDictionary("PORT");
    port_dict->SetValue("PORT_NUMBER", IntToString((*iter)->PortId()));
    port_dict->SetValue("PORT_ID", (*iter)->UniqueId());
    port_dict->SetValue("CAPABILITY", IsInputPort<PortClass>() ? "IN" : "OUT");
    port_dict->SetValue("DESCRIPTION", (*iter)->Description());

    if ((*iter)->PriorityCapability() != CAPABILITY_NONE) {
      TemplateDictionary *priority_dict =
        port_dict->AddSectionDictionary("SUPPORTS_PRIORITY");
      priority_dict->SetValue("PRIORITY", IntToString((*iter)->GetPriority()));
      if ((*iter)->PriorityCapability() == CAPABILITY_FULL) {
        TemplateDictionary *priority_mode_dict =
          priority_dict->AddSectionDictionary("SUPPORTS_PRIORITY_MODE");
        priority_mode_dict->SetValue("PRIORITY_MODE",
                                     IntToString((*iter)->GetPriorityMode()));
      }
    }

    Universe *universe = (*iter)->GetUniverse();
    if (universe)
      port_dict->SetValue("UNIVERSE", IntToString(universe->UniverseId()));

    if (*offset % 2)
      port_dict->ShowSection("ODD");
    (*offset)++;
    iter++;
  }
}
}  // ola
