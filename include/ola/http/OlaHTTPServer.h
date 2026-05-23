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
 * OlaHTTPServer.h
 * A HTTP Server with export map integration.
 * Copyright (C) 2012 Simon Newton
 */


#ifndef INCLUDE_OLA_HTTP_OLAHTTPSERVER_H_
#define INCLUDE_OLA_HTTP_OLAHTTPSERVER_H_

#include <ola/Clock.h>
#include <ola/ExportMap.h>
#include <ola/base/Macro.h>
#include <ola/http/HTTPServer.h>
#include <memory>
#include <string>

namespace ola {
namespace http {

/*
 * A HTTP Server with ExportMap support. You can inherit from this class to
 * implement specific handlers.
 */
class OlaHTTPServer {
 public:
    OlaHTTPServer(const HTTPServer::HTTPServerOptions &options,
                  ola::ExportMap *export_map);
    virtual ~OlaHTTPServer() {}

    virtual bool Init();
    bool Start() { return m_server.Start(); }
    void Stop() { return m_server.Stop(); }

 protected:
    Clock m_clock;
    ola::ExportMap *m_export_map;
    HTTPServer m_server;
    TimeStamp m_start_time;

    /**
     * Register a static file to serve
     */
    void RegisterFile(const std::string &file,
                      const std::string &content_type) {
        m_server.RegisterFile("/" + file, file, content_type);
    }

 private:
    static const char K_DATA_DIR_VAR[];
    static const char K_UPTIME_VAR[];

    inline void RegisterHandler(
        const std::string &path,
        int (OlaHTTPServer::*method)(const HTTPRequest*, HTTPResponse*)) {
      m_server.RegisterHandler(
          path,
          ola::NewCallback<OlaHTTPServer,
                           int,
                           const HTTPRequest*,
                           HTTPResponse*>(
                               this,
                               method));
    }

    int DisplayDebug(const HTTPRequest *request, HTTPResponse *response);
    int DisplayHandlers(const HTTPRequest *request, HTTPResponse *response);

    DISALLOW_COPY_AND_ASSIGN(OlaHTTPServer);
};
}  // namespace http
}  // namespace ola
#endif  // INCLUDE_OLA_HTTP_OLAHTTPSERVER_H_
