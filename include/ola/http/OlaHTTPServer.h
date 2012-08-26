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
 * OlaHTTPServer.h
 * A HTTP Server with export map integration.
 * Copyright (C) 2012 Simon Newton
 */


#ifndef INCLUDE_OLA_HTTP_OLAHTTPSERVER_H_
#define INCLUDE_OLA_HTTP_OLAHTTPSERVER_H_

#include <ola/Clock.h>
#include <ola/ExportMap.h>
#include <ola/http/HTTPServer.h>
#include <string>

namespace ola {
namespace http {

using ola::ExportMap;
using std::string;
using ola::NewCallback;

/*
 * A HTTP Server with ExportMap support. You can inherit from this class to
 * implement specific handlers.
 */
class OlaHTTPServer {
  public:
    OlaHTTPServer(const HTTPServer::HTTPServerOptions &options,
                  ExportMap *export_map);
    virtual ~OlaHTTPServer() {}

    bool Start() { return m_server.Start(); }
    void Stop() { return m_server.Stop(); }

  protected:
    Clock m_clock;
    ExportMap *m_export_map;
    HTTPServer m_server;
    TimeStamp m_start_time;

    /**
     * Register a static file to serve
     */
    void RegisterFile(const string &file, const string &content_type) {
        m_server.RegisterFile("/" + file, file, content_type);
    }

  private :
    static const char K_DATA_DIR_VAR[];
    static const char K_UPTIME_VAR[];

    inline void RegisterHandler(
        const string &path,
        int (OlaHTTPServer::*method)(const HTTPRequest*, HTTPResponse*)) {
      m_server.RegisterHandler(
          path,
          NewCallback<OlaHTTPServer, int, const HTTPRequest*, HTTPResponse*>(
            this,
            method));
    }

    int DisplayDebug(const HTTPRequest *request, HTTPResponse *response);
    int DisplayHandlers(const HTTPRequest *request, HTTPResponse *response);
};
}  // http
}  // ola
#endif  // INCLUDE_OLA_HTTP_OLAHTTPSERVER_H_
