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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * HTTPServer.h
 * The Base HTTP Server class.
 * Copyright (C) 2005-2008 Simon Newton
 *
 * This is a simple HTTP Server built around libmicrohttpd. It runs in a
 * separate thread.
 *
 * Example:
 *   HTTPServer::HTTPServerOptions options;
 *   options.port = ...;
 *   HTTPServer server(options);
 *   server.Init();
 *   server.Run();
 *   // get on with life and later...
 *   server.Stop();
 */


#ifndef INCLUDE_OLA_HTTP_HTTPSERVER_H_
#define INCLUDE_OLA_HTTP_HTTPSERVER_H_

#include <ola/Callback.h>
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServer.h>
#include <ola/thread/Thread.h>
#include <ola/web/Json.h>
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
namespace http {

using std::map;
using std::multimap;
using std::string;
using std::vector;

/*
 * Represents the HTTP request
 */
class HTTPRequest {
  public:
    HTTPRequest(const string &url,
                const string &method,
                const string &version,
                struct MHD_Connection *connection);
    ~HTTPRequest();
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
class HTTPResponse {
  public:
    explicit HTTPResponse(struct MHD_Connection *connection):
      m_connection(connection),
      m_status_code(MHD_HTTP_OK) {}

    void Append(const string &data) { m_data.append(data); }
    void SetContentType(const string &type);
    void SetHeader(const string &key, const string &value);
    void SetStatus(unsigned int status) { m_status_code = status; }
    void SetNoCache();
    int SendJson(const ola::web::JsonValue &json);
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
class HTTPServer: public ola::thread::Thread {
  public:
    typedef ola::Callback2<int, const HTTPRequest*, HTTPResponse*>
      BaseHTTPCallback;

    struct HTTPServerOptions {
      public:
        // The port to listen on
        uint16_t port;
        // The root for content served with ServeStaticContent();
        string data_dir;

        HTTPServerOptions()
          : port(0),
            data_dir("") {
        }
    };

    explicit HTTPServer(const HTTPServerOptions &options);
    virtual ~HTTPServer();
    bool Init();
    void *Run();
    void Stop();
    void UpdateSockets();

    /**
     * Called when there is HTTP IO activity to deal with. This is a noop as
     * MHD_run is called in UpdateSockets above.
     */
    void HandleHTTPIO() {}

    int DispatchRequest(const HTTPRequest *request, HTTPResponse *response);

    // Register a callback handler.
    bool RegisterHandler(const string &path, BaseHTTPCallback *handler);

    // Register a file handler.
    bool RegisterFile(const string &path,
                      const string &content_type);
    bool RegisterFile(const string &path,
                      const string &file,
                      const string &content_type);
    // Set the default handler.
    void RegisterDefaultHandler(BaseHTTPCallback *handler);

    void Handlers(vector<string> *handlers) const;
    const string DataDir() const { return m_data_dir; }

    // Return an error
    int ServeError(HTTPResponse *response, const string &details="");
    int ServeNotFound(HTTPResponse *response);

    // Return the contents of a file.
    int ServeStaticContent(const string &path,
                           const string &content_type,
                           HTTPResponse *response);

    static const char CONTENT_TYPE_PLAIN[];
    static const char CONTENT_TYPE_HTML[];
    static const char CONTENT_TYPE_GIF[];
    static const char CONTENT_TYPE_PNG[];
    static const char CONTENT_TYPE_CSS[];
    static const char CONTENT_TYPE_JS[];

    // Expose the SelectServer
    ola::io::SelectServer *SelectServer() { return &m_select_server; }

  private :
    typedef struct {
      string file_path;
      string content_type;
    } static_file_info;

    typedef std::set<ola::io::UnmanagedFileDescriptor*,
                     ola::io::UnmanagedFileDescriptor_lt> SocketSet;

    struct MHD_Daemon *m_httpd;
    ola::io::SelectServer m_select_server;
    SocketSet m_sockets;

    map<string, BaseHTTPCallback*> m_handlers;
    map<string, static_file_info> m_static_content;
    BaseHTTPCallback *m_default_handler;
    unsigned int m_port;
    string m_data_dir;


    HTTPServer(const HTTPServer&);
    HTTPServer& operator=(const HTTPServer&);

    int ServeStaticContent(static_file_info *file_info,
                           HTTPResponse *response);

    ola::io::UnmanagedFileDescriptor *NewSocket(fd_set *r_set,
                                                fd_set *w_set,
                                                int fd);
};
}  // http
}  // ola
#endif  // INCLUDE_OLA_HTTP_HTTPSERVER_H_
