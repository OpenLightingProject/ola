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
 * HttpServer.h
 * Interface for the OLA HTTP class
 * Copyright (C) 2005-2008 Simon Newton
 */


#ifndef OLAD_HTTPSERVER_H_
#define OLAD_HTTPSERVER_H_

#include <ola/Callback.h>
#include <ola/OlaThread.h>
#include <ola/network/SelectServer.h>
// 0.4.6 of microhttp doesn't include stdarg so we do it here.
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ola {

using std::map;
using std::multimap;
using std::string;
using std::vector;

/*
 * Represents the HTTP request
 */
class HttpRequest {
  public:
    HttpRequest(const string &url,
                const string &method,
                const string &version,
                struct MHD_Connection *connection);
    ~HttpRequest();
    bool Init();

    // accessors
    const string Url() const { return m_url; }
    const string Method() const { return m_method; }
    const string Version() const { return m_version; }

    void AddHeader(const string &key, const string &value);
    void AddPostParameter(const string &key, const string &value);
    void ProcessPostData(const char *data, size_t *data_size);
    const string GetHeader(const string &key) const;
    const string GetParameter(const string &key) const;
    const string GetPostParameter(const string &key) const;

    bool InFlight() const { return m_in_flight; }
    void SetInFlight() { m_in_flight = true; }

  private:
    string m_url;
    string m_method;
    string m_version;
    struct MHD_Connection *m_connection;
    map<string, string> m_headers;
    map<string, string> m_post_params;
    struct MHD_PostProcessor *m_processor;
    bool m_in_flight;

    static const unsigned int K_POST_BUFFER_SIZE = 1024;
};


/*
 * Represents the HTTP Response
 */
class HttpResponse {
  public:
    explicit HttpResponse(struct MHD_Connection *connection):
      m_connection(connection),
      m_status_code(MHD_HTTP_OK) {}

    void Append(const string &data) { m_data.append(data); }
    void SetContentType(const string &type);
    void SetHeader(const string &key, const string &value);
    void SetStatus(unsigned int status) { m_status_code = status; }
    int Send();
    struct MHD_Connection *Connection() const { return m_connection; }
  private:
    string m_data;
    struct MHD_Connection *m_connection;
    multimap<string, string> m_headers;
    unsigned int m_status_code;
};


/*
 * The base HTTP Server
 */
class HttpServer: public OlaThread {
  public:
    typedef ola::Callback2<int, const HttpRequest*, HttpResponse*>
      BaseHttpClosure;

    HttpServer(unsigned int port, const string &data_dir);
    virtual ~HttpServer();
    bool Init();
    void *Run();
    void Stop();
    void UpdateSockets();
    void HandleHTTPIO();

    int DispatchRequest(const HttpRequest *request, HttpResponse *response);
    bool RegisterHandler(const string &path, BaseHttpClosure *handler);
    bool RegisterFile(const string &path,
                      const string &file,
                      const string &content_type="");
    void RegisterDefaultHandler(BaseHttpClosure *handler);
    vector<string> Handlers() const;
    const string DataDir() const { return m_data_dir; }
    int ServeError(HttpResponse *response, const string &details="");
    int ServeNotFound(HttpResponse *response);

    typedef struct {
      string file_path;
      string content_type;
    } static_file_info;

    int ServeStaticContent(static_file_info *file_info,
                           HttpResponse *response);

    static const char CONTENT_TYPE_PLAIN[];
    static const char CONTENT_TYPE_HTML[];
    static const char CONTENT_TYPE_GIF[];
    static const char CONTENT_TYPE_PNG[];
    static const char CONTENT_TYPE_CSS[];
    static const char CONTENT_TYPE_JS[];

    // Expose the SelectServer
    ola::network::SelectServer *SelectServer() const {
      return m_select_server;
    }

  private :
    HttpServer(const HttpServer&);
    HttpServer& operator=(const HttpServer&);

    struct unmanaged_socket_lt {
      bool operator()(const ola::network::UnmanagedSocket *s1,
                      const ola::network::UnmanagedSocket *s2) const {
        return s1->ReadDescriptor() < s2->ReadDescriptor();
      }
    };

    ola::network::UnmanagedSocket *NewSocket(fd_set *r_set,
                                             fd_set *w_set,
                                             int fd);

    struct MHD_Daemon *m_httpd;
    ola::network::SelectServer *m_select_server;

    std::set<ola::network::UnmanagedSocket*, unmanaged_socket_lt> m_sockets;

    map<string, BaseHttpClosure*> m_handlers;
    map<string, static_file_info> m_static_content;
    BaseHttpClosure *m_default_handler;
    unsigned int m_port;
    string m_data_dir;
};
}  // ola
#endif  // OLAD_HTTPSERVER_H_
