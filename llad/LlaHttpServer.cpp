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
 * LlaHttpServer.cpp
 * Lla HTTP class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <errno.h>
#include <string>
#include <iostream>
#include <algorithm>

#include <lla/DmxBuffer.h>
#include <lla/DmxUtils.h>
#include <lla/StringUtils.h>
#include <llad/Device.h>
#include <llad/Plugin.h>
#include <llad/Port.h>
#include <llad/Universe.h>
#include "DeviceManager.h"
#include "LlaHttpServer.h"
#include "PluginLoader.h"
#include "UniverseStore.h"

namespace lla {

using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using ctemplate::TemplateDictionary;
using ctemplate::TemplateNamelist;

const string LlaHttpServer::K_DATA_DIR_VAR = "http_data_dir";

RegisterTemplateFilename(MAIN_FILENAME, "show_main_page.tpl");
RegisterTemplateFilename(PLUGINS_FILENAME, "show_loaded_plugins.tpl");
RegisterTemplateFilename(PLUGIN_INFO_FILENAME, "show_plugin_info.tpl");
RegisterTemplateFilename(DEVICE_FILENAME, "show_loaded_devices.tpl");
RegisterTemplateFilename(UNIVERSE_FILENAME, "show_universe_settings.tpl");
RegisterTemplateFilename(CONSOLE_FILENAME, "show_dmx_console.tpl");

LlaHttpServer::LlaHttpServer(ExportMap *export_map,
                             SelectServer *ss,
                             UniverseStore *universe_store,
                             PluginLoader *plugin_loader,
                             DeviceManager *device_manager,
                             unsigned int port,
                             bool enable_quit,
                             const string &data_dir):
  m_server(port, data_dir),
  m_export_map(export_map),
  m_ss(ss),
  m_universe_store(universe_store),
  m_plugin_loader(plugin_loader),
  m_device_manager(device_manager),
  m_enable_quit(enable_quit) {

  RegisterHandler("/debug", &LlaHttpServer::DisplayDebug);
  RegisterHandler("/quit", &LlaHttpServer::DisplayQuit);
  RegisterHandler("/help", &LlaHttpServer::DisplayHandlers);
  RegisterHandler("/main", &LlaHttpServer::DisplayMain);
  RegisterHandler("/plugins", &LlaHttpServer::DisplayPlugins);
  RegisterHandler("/plugin", &LlaHttpServer::DisplayPluginInfo);
  RegisterHandler("/devices", &LlaHttpServer::DisplayDevices);
  RegisterHandler("/universes", &LlaHttpServer::DisplayUniverses);
  RegisterHandler("/console", &LlaHttpServer::DisplayConsole);
  RegisterHandler("/reload_templates", &LlaHttpServer::DisplayTemplateReload);
  RegisterHandler("/set_dmx", &LlaHttpServer::HandleSetDmx);
  RegisterHandler("/", &LlaHttpServer::DisplayIndex);
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

  // warn on any missing templates
  TemplateNamelist::GetMissingList(false);
}


/*
 * Display the index page
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int LlaHttpServer::DisplayIndex(const HttpRequest *request,
                                HttpResponse *response) {
  HttpServer::static_file_info file_info;
  file_info.file_path = "index.html";
  file_info.content_type = HttpServer::CONTENT_TYPE_HTML;
  return m_server.ServeStaticContent(&file_info, response);
}


/*
 * Display the main page
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int LlaHttpServer::DisplayMain(const HttpRequest *request,
                               HttpResponse *response) {

  TemplateDictionary dict("main");

  if (m_enable_quit)
    dict.ShowSection("QUIT_ENABLED");
  return m_server.DisplayTemplate(MAIN_FILENAME, &dict, response);
}



/*
 * Display the plugins page.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int LlaHttpServer::DisplayPlugins(const HttpRequest *request,
                                  HttpResponse *response) {

  //dict->SetValueAndShowSection("USERNAME", username, "CHANGE_USER");
  TemplateDictionary dict("plugins");
  vector<AbstractPlugin*> plugins = m_plugin_loader->Plugins();
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
  } else
    dict.ShowSection("NO_PLUGINS");
  return m_server.DisplayTemplate(PLUGINS_FILENAME, &dict, response);
}


/*
 * Display the info for a single plugin.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int LlaHttpServer::DisplayPluginInfo(const HttpRequest *request,
                                     HttpResponse *response) {


  string val = request->GetParameter("id");
  int plugin_id = atoi(val.data());
  AbstractPlugin *plugin = NULL;
  if (plugin_id > 0 && plugin_id < LLA_PLUGIN_LAST)
    plugin = m_plugin_loader->GetPlugin((lla_plugin_id) plugin_id);

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
int LlaHttpServer::DisplayDevices(const HttpRequest *request,
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
  } else
    dict.ShowSection("NO_DEVICES");
  return m_server.DisplayTemplate(DEVICE_FILENAME, &dict, response);
}


/*
 * Show the universe settings page.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int LlaHttpServer::DisplayUniverses(const HttpRequest *request,
                                    HttpResponse *response) {

  TemplateDictionary dict("universes");
  vector<Universe*> *universes = m_universe_store->GetList();

  string action = request->GetParameter("action");
  bool save_changes = !action.empty();

  if (universes->size()) {
    vector<Universe*>::const_iterator iter;
    int i = 1;
    for (iter = universes->begin(); iter != universes->end(); ++iter) {

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
  } else
    dict.ShowSection("NO_UNIVERSES");

  delete universes;
  return m_server.DisplayTemplate(UNIVERSE_FILENAME, &dict, response);
}


/*
 * Show the Dmx console page.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int LlaHttpServer::DisplayConsole(const HttpRequest *request,
                                  HttpResponse *response) {

  string uni_id = request->GetParameter("u");
  errno = 0;
  int universe_id = atoi(uni_id.data());
  if (universe_id == 0 && errno != 0)
    return m_server.ServeNotFound(response);

  Universe *universe = m_universe_store->GetUniverse(universe_id);

  if (!universe)
    return m_server.ServeNotFound(response);

  TemplateDictionary dict("console");
  dict.SetValue("ID", IntToString(universe->UniverseId()));
  dict.SetValue("NAME", universe->Name());

  for (unsigned int i=0; i <= K_CONSOLE_SLIDERS; i++) {
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
int LlaHttpServer::HandleSetDmx(const HttpRequest *request,
                                HttpResponse *response) {

  string dmx_data_str = request->GetPostParameter("d");
  string uni_id = request->GetPostParameter("u");
  int universe_id = atoi(uni_id.data());
  errno = 0;
  if (universe_id == 0 && errno != 0)
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
int LlaHttpServer::DisplayDebug(const HttpRequest *request,
                                HttpResponse *response) {
  vector<BaseVariable*> variables = m_export_map->AllVariables();
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);

  vector<BaseVariable*>::iterator iter;
  for (iter = variables.begin(); iter != variables.end(); ++iter) {
    stringstream out;
    out << (*iter)->Name() << ": " << (*iter)->Value() << "\n";
    response->Append(out.str());
  }
  return response->Send();
}


/*
 * Display the index page
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int LlaHttpServer::DisplayQuit(const HttpRequest *request,
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
}


/*
 * Handle the template reload.
 */
int LlaHttpServer::DisplayTemplateReload(const HttpRequest *request,
                                         HttpResponse *response) {
  ctemplate::Template::ReloadAllIfChanged();
  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append("ok");
  return response->Send();
}


/*
 * Display a list of registered handlers
 */
int LlaHttpServer::DisplayHandlers(const HttpRequest *request,
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
}


/*
 * Register a handler
 */
inline void LlaHttpServer::RegisterHandler(
    const string &path,
    int (LlaHttpServer::*method)(const HttpRequest*, HttpResponse*)) {
  m_server.RegisterHandler(path, NewHttpClosure(this, method));
}


/*
 * Register a static file
 */
inline void LlaHttpServer::RegisterFile(const string &file,
                                 const string &content_type) {
  m_server.RegisterFile("/" + file, file, content_type);
}

/*
 * Populate a dictionary for this device.
 * @param dict the dictionary to fill
 * @param device the device to use
 */
void LlaHttpServer::PopulateDeviceDict(const HttpRequest *request,
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

  const vector<AbstractPort*> ports = device->Ports();
  vector<AbstractPort*>::const_iterator port_iter;
  int i = 1;
  for (port_iter = ports.begin(); port_iter != ports.end(); ++port_iter) {

    if (save_changes) {
      string variable_name = (*port_iter)->UniqueId();
      string uni_id = request->GetPostParameter(variable_name);
      Universe *universe = (*port_iter)->GetUniverse();
      errno = 0;
      int universe_id = atoi(uni_id.data());
      if (!uni_id.empty() && (universe_id != 0 || errno == 0)) {
        // valid number, patch this universe
        Universe *new_universe =
          m_universe_store->GetUniverseOrCreate(universe_id);

        if (new_universe) {
          if (universe == NULL || not (*universe == *new_universe))
            new_universe->AddPort(*port_iter);
        }
      } else {
        // invalid or blank, unpatch if needed
        if (universe)
          universe->RemovePort(*port_iter);
      }
    }

    TemplateDictionary *port_dict = dict->AddSectionDictionary("PORT");
    port_dict->SetValue("PORT_NUMBER", IntToString((*port_iter)->PortId()));
    port_dict->SetValue("PORT_ID", (*port_iter)->UniqueId());
    string capability;

    if ((*port_iter)->CanRead()) {
      capability += "IN";
      if ((*port_iter)->CanWrite())
        capability += " / ";
    }
    if ((*port_iter)->CanWrite())
      capability += "OUT";
    port_dict->SetValue("CAPABILITY", capability);
    port_dict->SetValue("DESCRIPTION", (*port_iter)->Description());

    Universe *universe = (*port_iter)->GetUniverse();
    if (universe)
      port_dict->SetValue("UNIVERSE", IntToString(universe->UniverseId()));

    if (i % 2)
      port_dict->ShowSection("ODD");
    i++;
  }
}


} //lla
