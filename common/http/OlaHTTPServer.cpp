/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * OlaHTTPServer.cpp
 * A HTTP Server with export map integration.
 * Copyright (C) 2012 Simon Newton
 */


#include <ola/http/OlaHTTPServer.h>
#include <ola/ExportMap.h>
#include <ola/Clock.h>
#include <memory>
#include <string>
#include <vector>

namespace ola {
namespace http {

using ola::ExportMap;
using std::auto_ptr;
using std::ostringstream;
using std::string;
using std::vector;

const char OlaHTTPServer::K_DATA_DIR_VAR[] = "http_data_dir";
const char OlaHTTPServer::K_UPTIME_VAR[] = "uptime-in-ms";

/**
 * Create a new OlaHTTPServer.
 * @param options The HTTPServerOptions options,
 * @param export_map the ExportMap to server
 */
OlaHTTPServer::OlaHTTPServer(const HTTPServer::HTTPServerOptions &options,
                             ExportMap *export_map)
    : m_export_map(export_map),
      m_server(options) {
  RegisterHandler("/debug", &OlaHTTPServer::DisplayDebug);
  RegisterHandler("/help", &OlaHTTPServer::DisplayHandlers);

  StringVariable *data_dir_var = export_map->GetStringVar(K_DATA_DIR_VAR);
  data_dir_var->Set(m_server.DataDir());
  m_clock.CurrentMonotonicTime(&m_start_time);
  export_map->GetStringVar(K_UPTIME_VAR);
}


/**
 * Setup the OLA HTTP server
 * @return true if this worked, false otherwise.
 */
bool OlaHTTPServer::Init() {
  return m_server.Init();
}


/**
 * Display the contents of the ExportMap
 */
int OlaHTTPServer::DisplayDebug(const HTTPRequest*,
                                HTTPResponse *raw_response) {
  auto_ptr<HTTPResponse> response(raw_response);
  ola::TimeStamp now;
  m_clock.CurrentMonotonicTime(&now);
  ola::TimeInterval diff = now - m_start_time;
  ostringstream str;
  str << diff.InMilliSeconds();
  m_export_map->GetStringVar(K_UPTIME_VAR)->Set(str.str());

  vector<BaseVariable*> variables = m_export_map->AllVariables();
  response->SetContentType(HTTPServer::CONTENT_TYPE_PLAIN);

  vector<BaseVariable*>::iterator iter;
  for (iter = variables.begin(); iter != variables.end(); ++iter) {
    ostringstream out;
    out << (*iter)->Name() << ": " << (*iter)->Value() << "\n";
    response->Append(out.str());
  }
  int r = response->Send();
  return r;
}


/**
 * Display a list of registered handlers
 */
int OlaHTTPServer::DisplayHandlers(const HTTPRequest*,
                                   HTTPResponse *raw_response) {
  auto_ptr<HTTPResponse> response(raw_response);
  vector<string> handlers;
  m_server.Handlers(&handlers);
  vector<string>::const_iterator iter;
  response->SetContentType(HTTPServer::CONTENT_TYPE_HTML);
  response->Append("<html><body><b>Registered Handlers</b><ul>");
  for (iter = handlers.begin(); iter != handlers.end(); ++iter) {
    response->Append("<li><a href='" + *iter + "'>" + *iter + "</a></li>");
  }
  response->Append("</ul></body></html>");
  int r = response->Send();
  return r;
}
}  // namespace http
}  // namespace ola
