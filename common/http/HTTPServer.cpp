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
 * HTTPServer.cpp
 * The base HTTP Server class.
 * Copyright (C) 2005 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <stdio.h>
#include <ola/Logging.h>
#include <ola/base/Macro.h>
#include <ola/file/Util.h>
#include <ola/http/HTTPServer.h>
#include <ola/io/Descriptor.h>
#include <ola/web/Json.h>
#include <ola/web/JsonWriter.h>

#ifdef _WIN32
#include <ola/win/CleanWinSock2.h>
#endif  // _WIN32

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ola {
namespace http {

#ifdef _WIN32
class UnmanagedSocketDescriptor : public ola::io::UnmanagedFileDescriptor {
 public:
  explicit UnmanagedSocketDescriptor(int fd) :
      ola::io::UnmanagedFileDescriptor(fd) {
    m_handle.m_type = ola::io::SOCKET_DESCRIPTOR;
    // Set socket to nonblocking to enable WSAEventSelect
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(UnmanagedSocketDescriptor);
};
#endif  // _WIN32

using std::ifstream;
using std::map;
using std::pair;
using std::set;
using std::string;
using std::vector;
using ola::io::UnmanagedFileDescriptor;
using ola::web::JsonValue;
using ola::web::JsonWriter;

const char HTTPServer::CONTENT_TYPE_PLAIN[] = "text/plain";
const char HTTPServer::CONTENT_TYPE_HTML[] = "text/html";
const char HTTPServer::CONTENT_TYPE_GIF[] = "image/gif";
const char HTTPServer::CONTENT_TYPE_PNG[] = "image/png";
const char HTTPServer::CONTENT_TYPE_CSS[] = "text/css";
const char HTTPServer::CONTENT_TYPE_JS[] = "text/javascript";
const char HTTPServer::CONTENT_TYPE_OCT[] = "application/octet-stream";


/**
 * @brief Called by MHD_get_connection_values to add headers to a request
 * object.
 * @param cls a pointer to an HTTPRequest object.
 * @param kind the source of the key-value pair
 * @param key the header name
 * @param value the header value
 */
static MHD_RESULT AddHeaders(void *cls, OLA_UNUSED enum MHD_ValueKind kind,
                             const char *key, const char *value) {
  HTTPRequest *request = static_cast<HTTPRequest*>(cls);
  string key_string = key;
  string value_string = value;
  request->AddHeader(key, value);
  return MHD_YES;
}


/**
 * @brief Called by MHD_create_post_processor to iterate over the post form data
 * @param request_cls a pointer to a HTTPRequest object
 * @param kind the source of the key-value pair
 * @param key the header name
 * @param filename the name of the uploaded file or NULL if unknown
 * @param content_type the MIME type of the data or NULL if unknown
 * @param transfer_encoding the encoding of the data or NULL if unknown
 * @param data the header value
 * @param off the offset of the data
 * @param size the number of bytes available
 */
MHD_RESULT IteratePost(void *request_cls, OLA_UNUSED enum MHD_ValueKind kind,
                       const char *key, OLA_UNUSED const char *filename,
                       OLA_UNUSED const char *content_type,
                       OLA_UNUSED const char *transfer_encoding,
                       const char *data,
                       OLA_UNUSED uint64_t off, OLA_UNUSED size_t size) {
  // libmicrohttpd has a bug where the size isn't set correctly.
  HTTPRequest *request = static_cast<HTTPRequest*>(request_cls);
  string value(data);
  request->AddPostParameter(key, value);
  return MHD_YES;
}


/**
 * @brief Called whenever a new request is made.
 *
 * This sets up HTTPRequest & HTTPResponse objects and then calls
 * DispatchRequest.
 */
static MHD_RESULT HandleRequest(void *http_server_ptr,
                                struct MHD_Connection *connection,
                                const char *url,
                                const char *method,
                                const char *version,
                                const char *upload_data,
                                size_t *upload_data_size,
                                void **ptr) {
  HTTPServer *http_server = static_cast<HTTPServer*>(http_server_ptr);
  HTTPRequest *request;

  // on the first call ptr is null
  if (*ptr == NULL) {
    request = new HTTPRequest(url, method, version, connection);
    if (!request) {
      return MHD_NO;
    }

    if (!request->Init()) {
      delete request;
      return MHD_NO;
    }
    *ptr = static_cast<void*>(request);
    return MHD_YES;
  }

  request = static_cast<HTTPRequest*>(*ptr);

  if (request->InFlight()) {
    // don't dispatch more than once
    return MHD_YES;
  }

  if (request->Method() == MHD_HTTP_METHOD_GET) {
    HTTPResponse *response = new HTTPResponse(connection);
    request->SetInFlight();
    return static_cast<MHD_RESULT>(
      http_server->DispatchRequest(request, response));

  } else if (request->Method() == MHD_HTTP_METHOD_POST) {
    if (*upload_data_size != 0) {
      request->ProcessPostData(upload_data, upload_data_size);
      *upload_data_size = 0;
      return MHD_YES;
    }
    request->SetInFlight();
    HTTPResponse *response = new HTTPResponse(connection);
    return static_cast<MHD_RESULT>(
      http_server->DispatchRequest(request, response));
  }
  return MHD_NO;
}


/**
 * @brief Called when a request completes.
 *
 * This deletes the associated HTTPRequest object.
 */
void RequestCompleted(void*,
                      struct MHD_Connection*,
                      void **request_cls,
                      enum MHD_RequestTerminationCode) {
  if (!request_cls) {
    return;
  }

  delete static_cast<HTTPRequest*>(*request_cls);
  *request_cls = NULL;
}


/*
 * HTTPRequest object
 * Setup the header callback and the post processor if needed.
 */
HTTPRequest::HTTPRequest(const string &url,
                         const string &method,
                         const string &version,
                         struct MHD_Connection *connection):
  m_url(url),
  m_method(method),
  m_version(version),
  m_connection(connection),
  m_processor(NULL),
  m_in_flight(false) {
}


/*
 * @brief Initialize this request
 * @return true if successful, false otherwise.
 */
bool HTTPRequest::Init() {
  MHD_get_connection_values(m_connection, MHD_HEADER_KIND, AddHeaders, this);

  if (m_method == MHD_HTTP_METHOD_POST) {
    m_processor = MHD_create_post_processor(m_connection,
                                            K_POST_BUFFER_SIZE,
                                            IteratePost,
                                            static_cast<void*>(this));
    return m_processor;
  }
  return true;
}


/*
 * @brief Cleanup this request object
 */
HTTPRequest::~HTTPRequest() {
  if (m_processor) {
    MHD_destroy_post_processor(m_processor);
  }
}


/**
 * @brief Add a header to the request object.
 * @param key the header name
 * @param value the value of the header
 */
void HTTPRequest::AddHeader(const string &key, const string &value) {
  std::pair<string, string> pair(key, value);
  m_headers.insert(pair);
}


/**
 * @brief Add a post parameter.
 *
 * This can be called multiple times and the values will be appended.
 * @param key the parameter name
 * @param value the value
 */
void HTTPRequest::AddPostParameter(const string &key, const string &value) {
  map<string, string>::iterator iter = m_post_params.find(key);

  if (iter == m_post_params.end()) {
    std::pair<string, string> pair(key, value);
    m_post_params.insert(pair);
  } else {
    iter->second.append(value);
  }
}


/**
 * @brief Process post data
 */
void HTTPRequest::ProcessPostData(const char *data, size_t *data_size) {
  MHD_post_process(m_processor, data, *data_size);
}


/**
 * @brief Return the value of the header sent with this request
 * @param key the name of the header
 * @returns the value of the header or empty string if it doesn't exist.
 */
const string HTTPRequest::GetHeader(const string &key) const {
  map<string, string>::const_iterator iter = m_headers.find(key);

  if (iter == m_headers.end()) {
    return "";
  } else {
    return iter->second;
  }
}


/**
 * @brief Return the value of a url parameter
 * @param key the name of the parameter
 * @return the value of the parameter
 */
const string HTTPRequest::GetParameter(const string &key) const {
  const char *value = MHD_lookup_connection_value(m_connection,
                                                  MHD_GET_ARGUMENT_KIND,
                                                  key.c_str());
  if (value) {
    return string(value);
  } else {
    return string();
  }
}

/**
 * @brief Return whether an url parameter exists
 * @param key the name of the parameter
 * @return if the parameter exists
 */
bool HTTPRequest::CheckParameterExists(const string &key) const {
  const char *value = MHD_lookup_connection_value(m_connection,
                                                  MHD_GET_ARGUMENT_KIND,
                                                  key.c_str());
  if (value != NULL) {
    return true;
  } else {
    return false;
    /*
     * TODO(Peter): try and check the "trailer" ?key, only in since Tue Jul 17
     * 2012
     * const char *trailer = MHD_lookup_connection_value(m_connection,
     *                                                   MHD_GET_ARGUMENT_KIND,
     *                                                   NULL);
     */
  }
}

/**
 * @brief Lookup a post parameter in this request
 * @param key the name of the parameter
 * @return the value of the parameter or the empty string if it doesn't exist
 */
const string HTTPRequest::GetPostParameter(const string &key) const {
  map<string, string>::const_iterator iter = m_post_params.find(key);

  if (iter == m_post_params.end()) {
    return "";
  } else {
    return iter->second;
  }
}


/**
 * @brief Set the content-type header
 * @param type the content type
 * @return true if the header was set correctly, false otherwise
 */
void HTTPResponse::SetContentType(const string &type) {
  SetHeader(MHD_HTTP_HEADER_CONTENT_TYPE, type);
}


/**
 * @brief Set the appropriate headers so this response isn't cached
 */
void HTTPResponse::SetNoCache() {
  SetHeader(MHD_HTTP_HEADER_CACHE_CONTROL, "no-cache, must-revalidate");
}


/**
 * @brief Set a header in the response
 * @param key the header name
 * @param value the header value
 * @return true if the header was set correctly, false otherwise
 */
void HTTPResponse::SetHeader(const string &key, const string &value) {
  std::pair<string, string> pair(key, value);
  m_headers.insert(pair);
}


/**
 * @brief Send a JsonObject as the response.
 * @return true on success, false on error
 */
int HTTPResponse::SendJson(const JsonValue &json) {
  const string output = JsonWriter::AsString(json);
  struct MHD_Response *response = HTTPServer::BuildResponse(
      static_cast<void*>(const_cast<char*>(output.data())),
      output.length());
  HeadersMultiMap::const_iterator iter;
  for (iter = m_headers.begin(); iter != m_headers.end(); ++iter) {
    MHD_add_response_header(response,
                            iter->first.c_str(),
                            iter->second.c_str());
  }
  int ret = MHD_queue_response(m_connection, m_status_code, response);
  MHD_destroy_response(response);
  return ret;
}


/**
 * @brief Send the HTTP response
 * @return true on success, false on error
 */
int HTTPResponse::Send() {
  HeadersMultiMap::const_iterator iter;
  struct MHD_Response *response = HTTPServer::BuildResponse(
      static_cast<void*>(const_cast<char*>(m_data.data())),
      m_data.length());
  for (iter = m_headers.begin(); iter != m_headers.end(); ++iter) {
    MHD_add_response_header(response,
                            iter->first.c_str(),
                            iter->second.c_str());
  }
  int ret = MHD_queue_response(m_connection, m_status_code, response);
  MHD_destroy_response(response);
  return ret;
}


/**
 * @brief Setup the HTTP server.
 * @param options the configuration options for the server
 */
HTTPServer::HTTPServer(const HTTPServerOptions &options)
    : Thread(Thread::Options("http")),
      m_httpd(NULL),
      m_default_handler(NULL),
      m_port(options.port),
      m_data_dir(options.data_dir) {
  ola::io::SelectServer::Options ss_options;
  // See issue #761. epoll/kqueue can't be used with the current
  // implementation.
  ss_options.force_select = true;
  m_select_server.reset(new ola::io::SelectServer(ss_options));
}


/**
 * @brief Destroy this object
 */
HTTPServer::~HTTPServer() {
  Stop();

  if (m_httpd)
    MHD_stop_daemon(m_httpd);

  map<string, BaseHTTPCallback*>::const_iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter) {
    delete iter->second;
  }

  if (m_default_handler) {
    delete m_default_handler;
    m_default_handler = NULL;
  }

  m_handlers.clear();
}


/**
 * @brief Setup the HTTP server
 * @return true on success, false on failure
 */
bool HTTPServer::Init() {
  if (m_httpd) {
    OLA_INFO << "Non null pointers found, Init() was probably called twice";
    return false;
  }

  m_httpd = MHD_start_daemon(MHD_NO_FLAG,
                             m_port,
                             NULL,
                             NULL,
                             &HandleRequest,
                             this,
                             MHD_OPTION_NOTIFY_COMPLETED,
                             RequestCompleted,
                             NULL,
                             MHD_OPTION_END);

  if (m_httpd) {
    m_select_server->RunInLoop(NewCallback(this, &HTTPServer::UpdateSockets));
  }

  return m_httpd ? true : false;
}


/**
 * @brief The entry point into the new thread
 */
void *HTTPServer::Run() {
  if (!m_httpd) {
    OLA_WARN << "HTTPServer::Run called but the server wasn't setup.";
    return NULL;
  }

  OLA_INFO << "HTTP Server started on port " << m_port;

#ifdef _WIN32
  // set a short poll interval since we'd block too long otherwise.
  // TODO(Lukas) investigate why the poller does not wake up on HTTP requests.
  m_select_server->SetDefaultInterval(TimeInterval(1, 0));
#else
  // set a long poll interval so we don't spin
  m_select_server->SetDefaultInterval(TimeInterval(60, 0));
#endif  // _WIN32
  m_select_server->Run();

  // clean up any remaining sockets
  SocketSet::iterator iter = m_sockets.begin();
  for (; iter != m_sockets.end(); ++iter) {
    FreeSocket(*iter);
  }
  m_sockets.clear();
  return NULL;
}


/**
 * @brief Stop the HTTP server
 */
void HTTPServer::Stop() {
  if (IsRunning()) {
    OLA_INFO << "Notifying HTTP server thread to stop";
    m_select_server->Terminate();
    OLA_INFO << "Waiting for HTTP server thread to exit";
    Join();
    OLA_INFO << "HTTP server thread exited";
  }
}


/**
 * @brief This is run every loop iteration to update the list of sockets in the
 * SelectServer from MHD.
 */
void HTTPServer::UpdateSockets() {
  // We always call MHD_run so we send any queued responses. This isn't
  // inefficient because the only thing that can wake up the select server is
  // activity on a http socket or the client socket. The latter almost always
  // results in a change to HTTP state.
  if (MHD_run(m_httpd) == MHD_NO) {
    OLA_WARN << "MHD run failed";
  }

  fd_set r_set, w_set, e_set;
  int max_fd = 0;
  FD_ZERO(&r_set);
  FD_ZERO(&w_set);
#ifdef MHD_SOCKET_DEFINED
  if (MHD_YES != MHD_get_fdset(m_httpd, &r_set, &w_set, &e_set,
                               reinterpret_cast<MHD_socket*>(&max_fd))) {
#else
  if (MHD_YES != MHD_get_fdset(m_httpd, &r_set, &w_set, &e_set, &max_fd)) {
#endif  // MHD_SOCKET_DEFINED
    OLA_WARN << "Failed to get a list of the file descriptors for MHD";
    return;
  }

  SocketSet::iterator iter = m_sockets.begin();

  // This isn't the best plan, talk to the MHD devs about exposing the list of
  // FD in a more suitable way
  int i = 0;
  while (iter != m_sockets.end() && i <= max_fd) {
    DescriptorState *state = *iter;
    if (ola::io::ToFD(state->descriptor->ReadDescriptor()) < i) {
      // This socket is no longer required so remove it
      FreeSocket(state);
      m_sockets.erase(iter++);
    } else if (ola::io::ToFD(state->descriptor->ReadDescriptor()) == i) {
      // Check if this socket must be updated.
      if (FD_ISSET(i, &r_set) && state->read == 0) {
        m_select_server->AddReadDescriptor(state->descriptor);
        state->read = 1;
      } else if ((!FD_ISSET(i, &r_set)) && state->read == 1) {
        m_select_server->RemoveReadDescriptor(state->descriptor);
        state->read = 0;
      }

      if (FD_ISSET(i, &w_set) && state->write == 0) {
        m_select_server->AddWriteDescriptor(state->descriptor);
        state->write = 1;
      } else if ((!FD_ISSET(i, &w_set)) && state->write == 1) {
        m_select_server->RemoveWriteDescriptor(state->descriptor);
        state->write = 0;
      }
      iter++;
      i++;
    } else {
      // this is a new socket
      if (FD_ISSET(i, &r_set) || FD_ISSET(i, &w_set)) {
        InsertSocket(FD_ISSET(i, &r_set), FD_ISSET(i, &w_set), i);
      }
      i++;
    }
  }

  while (iter != m_sockets.end()) {
    FreeSocket(*iter);
    m_sockets.erase(iter++);
  }

  for (; i <= max_fd; i++) {
    // add the remaining sockets to the SS
    if (FD_ISSET(i, &r_set) || FD_ISSET(i, &w_set)) {
      InsertSocket(FD_ISSET(i, &r_set), FD_ISSET(i, &w_set), i);
    }
  }
}


/**
 * @brief Call the appropriate handler.
 */
int HTTPServer::DispatchRequest(const HTTPRequest *request,
                                HTTPResponse *response) {
  map<string, BaseHTTPCallback*>::iterator iter =
    m_handlers.find(request->Url());

  if (iter != m_handlers.end()) {
    return iter->second->Run(request, response);
  }

  map<string, static_file_info>::iterator file_iter =
    m_static_content.find(request->Url());

  if (file_iter != m_static_content.end()) {
    return ServeStaticContent(&(file_iter->second), response);
  }

  if (m_default_handler) {
    return m_default_handler->Run(request, response);
  }

  return ServeNotFound(response);
}


/**
 * @brief Register a handler
 * @param path the url to respond on
 * @param handler the Closure to call for this request. These will be freed
 * once the HTTPServer is destroyed.
 */
bool HTTPServer::RegisterHandler(const string &path,
                                 BaseHTTPCallback *handler) {
  map<string, BaseHTTPCallback*>::const_iterator iter = m_handlers.find(path);
  if (iter != m_handlers.end()) {
    return false;
  }
  pair<string, BaseHTTPCallback*> pair(path, handler);
  m_handlers.insert(pair);
  return true;
}


/**
 * @brief Register a static file. The root of the URL corresponds to the data dir.
 * @param path the URL path for the file e.g. '/foo.png'
 * @param content_type the content type.
 */
bool HTTPServer::RegisterFile(const std::string &path,
                              const std::string &content_type) {
  if (path.empty() || path[0] != '/') {
    OLA_WARN << "Invalid static file: " << path;
    return false;
  }
  return RegisterFile(path, path.substr(1), content_type);
}


/**
 * @brief Register a static file
 * @param path the path to serve on e.g. /foo.png
 * @param file the path to the file to serve relative to the data dir e.g.
 * images/foo.png
 * @param content_type the content type.
 */
bool HTTPServer::RegisterFile(const std::string &path,
                              const std::string &file,
                              const std::string &content_type) {
  map<string, static_file_info>::const_iterator file_iter = (
      m_static_content.find(path));

  if (file_iter != m_static_content.end()) {
    return false;
  }

  static_file_info file_info;
  file_info.file_path = file;
  file_info.content_type = content_type;

  pair<string, static_file_info> pair(path, file_info);
  m_static_content.insert(pair);
  return true;
}


/**
 * @brief Set the default handler.
 * @param handler the default handler to call. This will be freed when the
 * HTTPServer is destroyed.
 */
void HTTPServer::RegisterDefaultHandler(BaseHTTPCallback *handler) {
  m_default_handler = handler;
}


/**
 * @brief Return a list of all handlers registered
 */
void HTTPServer::Handlers(vector<string> *handlers) const {
  map<string, BaseHTTPCallback*>::const_iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter) {
    handlers->push_back(iter->first);
  }

  map<string, static_file_info>::const_iterator file_iter;
  for (file_iter = m_static_content.begin();
       file_iter != m_static_content.end(); ++file_iter) {
    handlers->push_back(file_iter->first);
  }
}

/**
 * @brief Serve an error.
 * @param response the response to use.
 * @param details the error description
 */
int HTTPServer::ServeError(HTTPResponse *response, const string &details) {
  response->SetStatus(MHD_HTTP_INTERNAL_SERVER_ERROR);
  response->SetContentType(CONTENT_TYPE_HTML);
  response->Append("<b>500 Server Error</b>");
  if (!details.empty()) {
    response->Append("<p>");
    response->Append(details);
    response->Append("</p>");
  }
  int r = response->Send();
  delete response;
  return r;
}

/**
 * @brief Serve a 404
 * @param response the response to use
 */
int HTTPServer::ServeNotFound(HTTPResponse *response) {
  response->SetStatus(MHD_HTTP_NOT_FOUND);
  response->SetContentType(CONTENT_TYPE_HTML);
  response->Append("<b>404 Not Found</b>");
  int r = response->Send();
  delete response;
  return r;
}

/**
 * @brief Serve a redirect
 * @param response the response to use
 * @param location the location to redirect to
 */
int HTTPServer::ServeRedirect(HTTPResponse *response, const string &location) {
  response->SetStatus(MHD_HTTP_FOUND);
  response->SetContentType(CONTENT_TYPE_HTML);
  response->SetHeader(MHD_HTTP_HEADER_LOCATION, location);
  response->Append("<b>302 Found</b> See " + location);
  int r = response->Send();
  delete response;
  return r;
}

/**
 * @brief Return the contents of a file
 */
int HTTPServer::ServeStaticContent(const std::string &path,
                                   const std::string &content_type,
                                   HTTPResponse *response) {
  static_file_info file_info;
  file_info.file_path = path;
  file_info.content_type = content_type;
  return ServeStaticContent(&file_info, response);
}


/**
 * @brief Serve static content.
 * @param file_info details on the file to server
 * @param response the response to use
 */
int HTTPServer::ServeStaticContent(static_file_info *file_info,
                                   HTTPResponse *response) {
  char *data;
  unsigned int length;
  string file_path = m_data_dir;
  file_path.push_back(ola::file::PATH_SEPARATOR);
  file_path.append(file_info->file_path);
  ifstream i_stream(file_path.c_str(), ifstream::binary);

  if (!i_stream.is_open()) {
    OLA_WARN << "Missing file: " << file_path;
    return ServeNotFound(response);
  }

  i_stream.seekg(0, std::ios::end);
  length = i_stream.tellg();
  i_stream.seekg(0, std::ios::beg);

  data = static_cast<char*>(malloc(length));

  i_stream.read(data, length);
  i_stream.close();

  struct MHD_Response *mhd_response = BuildResponse(static_cast<void*>(data),
                                                    length);

  if (!file_info->content_type.empty()) {
    MHD_add_response_header(mhd_response,
                            MHD_HTTP_HEADER_CONTENT_TYPE,
                            file_info->content_type.c_str());
  }

  int ret = MHD_queue_response(response->Connection(),
                               MHD_HTTP_OK,
                               mhd_response);
  MHD_destroy_response(mhd_response);
  delete response;
  return ret;
}

void HTTPServer::InsertSocket(bool is_readable, bool is_writeable, int fd) {
#ifdef _WIN32
  UnmanagedSocketDescriptor *socket = new UnmanagedSocketDescriptor(fd);
#else
  UnmanagedFileDescriptor *socket = new UnmanagedFileDescriptor(fd);
#endif  // _WIN32
  socket->SetOnData(NewCallback(this, &HTTPServer::HandleHTTPIO));
  socket->SetOnWritable(NewCallback(this, &HTTPServer::HandleHTTPIO));

  DescriptorState *state = new DescriptorState(socket);

  if (is_readable) {
    m_select_server->AddReadDescriptor(state->descriptor);
    state->read = 1;
  }

  if (is_writeable) {
    state->write = 1;
    m_select_server->AddWriteDescriptor(state->descriptor);
  }

  m_sockets.insert(state);
}

void HTTPServer::FreeSocket(DescriptorState *state) {
  if (state->read) {
    m_select_server->RemoveReadDescriptor(state->descriptor);
  }

  if (state->write) {
    m_select_server->RemoveWriteDescriptor(state->descriptor);
  }
  delete state->descriptor;
  delete state;
}


struct MHD_Response *HTTPServer::BuildResponse(void *data, size_t size) {
#ifdef HAVE_MHD_CREATE_RESPONSE_FROM_BUFFER
  return MHD_create_response_from_buffer(size, data, MHD_RESPMEM_MUST_COPY);
#else
  return MHD_create_response_from_data(size, data, MHD_NO, MHD_YES);
#endif  // HAVE_MHD_CREATE_RESPONSE_FROM_BUFFER
}
}  // namespace http
}  // namespace ola
