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
 * HTTPServer.h
 * The Base HTTP Server class.
 * Copyright (C) 2005 Simon Newton
 */


#ifndef INCLUDE_OLA_HTTP_HTTPSERVER_H_
#define INCLUDE_OLA_HTTP_HTTPSERVER_H_

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServer.h>
#include <ola/thread/Thread.h>
#include <ola/web/Json.h>
// 0.4.6 of microhttp doesn't include stdarg so we do it here.
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <ola/win/CleanWinSock2.h>
#else
#include <sys/select.h>
#include <sys/socket.h>
#endif  // _WIN32
#include <microhttpd.h>
#include <map>
#include <set>
#include <string>
#include <vector>

// Beginning with v0.9.71, libmicrohttpd changed the return type of most
// functions from int to enum MHD_Result
// https://git.gnunet.org/gnunet.git/tree/src/include/gnunet_mhd_compat.h
// proposes to define a constant for the return type so it works well
// with all versions of libmicrohttpd
#if MHD_VERSION >= 0x00097002
#define MHD_RESULT enum MHD_Result
#else
#define MHD_RESULT int
#endif

namespace ola {
namespace http {

/*
 * Represents the HTTP request
 */
class HTTPRequest {
 public:
  HTTPRequest(const std::string &url,
              const std::string &method,
              const std::string &version,
              struct MHD_Connection *connection);
  ~HTTPRequest();
  bool Init();

  // accessors
  const std::string Url() const { return m_url; }
  const std::string Method() const { return m_method; }
  const std::string Version() const { return m_version; }

  void AddHeader(const std::string &key, const std::string &value);
  void AddPostParameter(const std::string &key, const std::string &value);
  void ProcessPostData(const char *data, size_t *data_size);
  const std::string GetHeader(const std::string &key) const;
  bool CheckParameterExists(const std::string &key) const;
  const std::string GetParameter(const std::string &key) const;
  const std::string GetPostParameter(const std::string &key) const;

  bool InFlight() const { return m_in_flight; }
  void SetInFlight() { m_in_flight = true; }

 private:
  std::string m_url;
  std::string m_method;
  std::string m_version;
  struct MHD_Connection *m_connection;
  std::map<std::string, std::string> m_headers;
  std::map<std::string, std::string> m_post_params;
  struct MHD_PostProcessor *m_processor;
  bool m_in_flight;

  static const unsigned int K_POST_BUFFER_SIZE = 1024;

  DISALLOW_COPY_AND_ASSIGN(HTTPRequest);
};


/*
 * Represents the HTTP Response
 */
class HTTPResponse {
 public:
  explicit HTTPResponse(struct MHD_Connection *connection):
    m_connection(connection),
    m_status_code(MHD_HTTP_OK) {}

  void Append(const std::string &data) { m_data.append(data); }
  void SetContentType(const std::string &type);
  void SetHeader(const std::string &key, const std::string &value);
  void SetStatus(unsigned int status) { m_status_code = status; }
  void SetNoCache();
  void SetAccessControlAllowOriginAll();
  int SendJson(const ola::web::JsonValue &json);
  int Send();
  struct MHD_Connection *Connection() const { return m_connection; }
 private:
  std::string m_data;
  struct MHD_Connection *m_connection;
  typedef std::multimap<std::string, std::string> HeadersMultiMap;
  HeadersMultiMap m_headers;
  unsigned int m_status_code;

  DISALLOW_COPY_AND_ASSIGN(HTTPResponse);
};


/**
 * @addtogroup http_server
 * @{
 * @class HTTPServer
 * @brief The base HTTP Server.
 *
 * This is a simple HTTP Server built around libmicrohttpd. It runs in a
 * separate thread.
 *
 * @examplepara
 * @code
 *   HTTPServer::HTTPServerOptions options;
 *   options.port = ...;
 *   HTTPServer server(options);
 *   server.Init();
 *   server.Run();
 *   // get on with life and later...
 *   server.Stop();
 * @endcode
 * @}
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
    std::string data_dir;

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
  bool RegisterHandler(const std::string &path, BaseHTTPCallback *handler);

  // Register a file handler.
  bool RegisterFile(const std::string &path,
                    const std::string &content_type);
  bool RegisterFile(const std::string &path,
                    const std::string &file,
                    const std::string &content_type);
  // Set the default handler.
  void RegisterDefaultHandler(BaseHTTPCallback *handler);

  void Handlers(std::vector<std::string> *handlers) const;
  const std::string DataDir() const { return m_data_dir; }

  // Return an error
  int ServeError(HTTPResponse *response, const std::string &details = "");
  int ServeNotFound(HTTPResponse *response);
  static int ServeRedirect(HTTPResponse *response, const std::string &location);

  // Return the contents of a file.
  int ServeStaticContent(const std::string &path,
                         const std::string &content_type,
                         HTTPResponse *response);

  static const char CONTENT_TYPE_PLAIN[];
  static const char CONTENT_TYPE_HTML[];
  static const char CONTENT_TYPE_GIF[];
  static const char CONTENT_TYPE_PNG[];
  static const char CONTENT_TYPE_ICO[];
  static const char CONTENT_TYPE_CSS[];
  static const char CONTENT_TYPE_JS[];
  static const char CONTENT_TYPE_OCT[];
  static const char CONTENT_TYPE_XML[];
  static const char CONTENT_TYPE_JSON[];

  // Expose the SelectServer
  ola::io::SelectServer *SelectServer() { return m_select_server.get(); }

  static struct MHD_Response *BuildResponse(void *data, size_t size);

 private :
  typedef struct {
    std::string file_path;
    std::string content_type;
  } static_file_info;

  struct DescriptorState {
   public:
    explicit DescriptorState(ola::io::UnmanagedFileDescriptor *_descriptor)
        : descriptor(_descriptor), read(0), write(0) {}

    ola::io::UnmanagedFileDescriptor *descriptor;
    uint8_t read    : 1;
    uint8_t write   : 1;
    uint8_t         : 6;
  };

  struct Descriptor_lt {
    bool operator()(const DescriptorState *d1,
                    const DescriptorState *d2) const {
      return d1->descriptor->ReadDescriptor() <
             d2->descriptor->ReadDescriptor();
    }
  };

  typedef std::set<DescriptorState*, Descriptor_lt> SocketSet;

  struct MHD_Daemon *m_httpd;
  std::auto_ptr<ola::io::SelectServer> m_select_server;
  SocketSet m_sockets;

  std::map<std::string, BaseHTTPCallback*> m_handlers;
  std::map<std::string, static_file_info> m_static_content;
  BaseHTTPCallback *m_default_handler;
  unsigned int m_port;
  std::string m_data_dir;

  int ServeStaticContent(static_file_info *file_info,
                         HTTPResponse *response);

  void InsertSocket(bool is_readable, bool is_writeable, int fd);
  void FreeSocket(DescriptorState *state);

  DISALLOW_COPY_AND_ASSIGN(HTTPServer);
};
}  // namespace http
}  // namespace ola
#endif  // INCLUDE_OLA_HTTP_HTTPSERVER_H_
