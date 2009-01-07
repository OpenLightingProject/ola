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

#include <string>
#include <iostream>
#include "LlaHttpServer.h"

namespace lla {

using std::cout;
using std::endl;
using std::string;
using std::stringstream;

const string LlaHttpServer::K_DATA_DIR_VAR = "http_data_dir";


LlaHttpServer::LlaHttpServer(ExportMap *export_map,
                             SelectServer *ss,
                             unsigned int port,
                             bool enable_quit,
                             const string &data_dir):
  HttpServer(port, data_dir),
  m_export_map(export_map),
  m_ss(ss),
  m_enable_quit(enable_quit) {

  RegisterHandler("/debug", NewHttpClosure(this, &LlaHttpServer::DisplayDebug));
  RegisterHandler("/quit", NewHttpClosure(this, &LlaHttpServer::DisplayQuit));
  RegisterHandler("/help", NewHttpClosure(this, &LlaHttpServer::DisplayHandlers));
  RegisterFile("/index.html", "index.html", CONTENT_TYPE_HTML);
  RegisterFile("/menu.html", "menu.html", CONTENT_TYPE_HTML);
  RegisterFile("/about.html", "about.html", CONTENT_TYPE_HTML);
  RegisterFile("/simple.css", "simple.css", CONTENT_TYPE_CSS);
  RegisterFile("/notice.gif", "notice.gif", CONTENT_TYPE_GIF);
  RegisterFile("/GPL.txt", "GPL.txt", CONTENT_TYPE_PLAIN);
  RegisterDefaultHandler(NewHttpClosure(this, &LlaHttpServer::DisplayIndex));

  StringVariable *data_dir_var = export_map->GetStringVar(K_DATA_DIR_VAR);
  data_dir_var->Set(m_data_dir);
}


/*
 * Display the index page
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int LlaHttpServer::DisplayIndex(const HttpRequest *request, HttpResponse *response) {
  static_file_info file_info;
  file_info.file_path = "index.html";
  file_info.content_type = CONTENT_TYPE_HTML;
  return ServeStaticContent(&file_info, response);
}


/*
 * Display the debug page
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int LlaHttpServer::DisplayDebug(const HttpRequest *request, HttpResponse *response) {
  vector<BaseVariable*> variables = m_export_map->AllVariables();
  response->SetContentType(CONTENT_TYPE_PLAIN);

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
int LlaHttpServer::DisplayQuit(const HttpRequest *request, HttpResponse *response) {
  if (m_enable_quit) {
    response->SetContentType(CONTENT_TYPE_PLAIN);
    response->Append("ok");
    m_ss->Terminate();
  } else {
    response->SetStatus(403);
    response->SetContentType(CONTENT_TYPE_HTML);
    response->Append("<b>403 Unauthorized</b>");
  }
  return response->Send();
}


/*
 * Display a list of registered handlers
 */
int LlaHttpServer::DisplayHandlers(const HttpRequest *request, HttpResponse *response) {
  vector<string> handlers = Handlers();
  vector<string>::const_iterator iter;
  response->SetContentType(CONTENT_TYPE_HTML);
  response->Append("<html><body><b>Registerd Handlers</b><ul>");
  for (iter = handlers.begin(); iter != handlers.end(); ++iter) {
    response->Append("<li><a href='" + *iter + "'>" + *iter + "</a></li>");
  }
  response->Append("</ul></body></html>");
  return response->Send();
}


} //lla
