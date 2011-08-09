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
 * HttpServer.cpp
 * Ola HTTP class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "olad/HttpServer.h"
#include "ola/Logging.h"

namespace ola {

using std::ifstream;
using std::pair;
using std::set;
using std::string;
using ola::network::UnmanagedSocket;

const char HttpServer::CONTENT_TYPE_PLAIN[] = "text/plain";
const char HttpServer::CONTENT_TYPE_HTML[] = "text/html";
const char HttpServer::CONTENT_TYPE_GIF[] = "image/gif";
const char HttpServer::CONTENT_TYPE_PNG[] = "image/png";
const char HttpServer::CONTENT_TYPE_CSS[] = "text/css";
const char HttpServer::CONTENT_TYPE_JS[] = "text/javascript";


/**
 * Called by MHD_get_connection_values to add headers to a request obect.
 * @param cls a pointer to an HttpRequest object.
 * @param key the header name
 * @param value the header value
 */
static int AddHeaders(void *cls, enum MHD_ValueKind kind, const char *key,
                      const char *value) {
  HttpRequest *request = static_cast<HttpRequest*>(cls);
  string key_string = key;
  string value_string = value;
  request->AddHeader(key, value);
  return MHD_YES;
  (void) kind;
}


/**
 * Called by MHD_create_post_processor to iterate over the post form data
 * @param request_cls a pointer to a HttpRequest object
 * @param key the header name
 * @param value the header value
 */
int IteratePost(void *request_cls, enum MHD_ValueKind kind, const char *key,
                const char *filename, const char *content_type,
                const char *transfer_encoding, const char *data, uint64_t off,
                size_t size) {
  // libmicrohttpd has a bug where the zie isn't set correctly.
  HttpRequest *request = static_cast<HttpRequest*>(request_cls);
  string value(data);
  request->AddPostParameter(key, value);
  return MHD_YES;
  (void) content_type;
  (void) filename;
  (void) kind;
  (void) transfer_encoding;
  (void) off;
  (void) size;
}


/**
 * Called whenever a new request is made. This sets up HttpRequest &
 * HttpResponse objects and then calls DispatchRequest.
 */
static int HandleRequest(void *http_server_ptr,
                         struct MHD_Connection *connection,
                         const char *url,
                         const char *method,
                         const char *version,
                         const char *upload_data,
                         size_t *upload_data_size,
                         void **ptr) {
  HttpServer *http_server = static_cast<HttpServer*>(http_server_ptr);
  HttpRequest *request;

  // on the first call ptr is null
  if (*ptr == NULL) {
    request = new HttpRequest(url, method, version, connection);
    if (!request)
      return MHD_NO;

    if (!request->Init()) {
      delete request;
      return MHD_NO;
    }
    *ptr = static_cast<void*>(request);
    return MHD_YES;
  }

  request = static_cast<HttpRequest*>(*ptr);

  if (request->InFlight())
    // don't dispatch more than once
    return MHD_YES;

  if (request->Method() == MHD_HTTP_METHOD_GET) {
    HttpResponse *response = new HttpResponse(connection);
    request->SetInFlight();
    return http_server->DispatchRequest(request, response);

  } else if (request->Method() == MHD_HTTP_METHOD_POST) {
    if (*upload_data_size != 0) {
      request->ProcessPostData(upload_data, upload_data_size);
      *upload_data_size = 0;
      return MHD_YES;
    }
    request->SetInFlight();
    HttpResponse *response = new HttpResponse(connection);
    return http_server->DispatchRequest(request, response);
  }
  return MHD_NO;
}


/**
 * Called when a request completes. This deletes the associated HttpRequest
 * object.
 */
void RequestCompleted(void *cls,
                      struct MHD_Connection *connection,
                      void **request_cls,
                      enum MHD_RequestTerminationCode toe) {
  HttpRequest *request = static_cast<HttpRequest*>(*request_cls);

  if (!request)
    return;

  delete request;
  *request_cls = NULL;
  (void) cls;
  (void) connection;
  (void) toe;
}


/*
 * HttpRequest object
 * Setup the header callback and the post processor if needed.
 */
HttpRequest::HttpRequest(const string &url,
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
 * Initialize this request
 * @return true if succesful, false otherwise.
 */
bool HttpRequest::Init() {
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
 * Cleanup this request object
 */
HttpRequest::~HttpRequest() {
  if (m_processor)
    MHD_destroy_post_processor(m_processor);
}


/*
 * Add a header to the request object.
 * @param key the header name
 * @param value the value of the header
 */
void HttpRequest::AddHeader(const string &key, const string &value) {
  std::pair<string, string> pair(key, value);
  m_headers.insert(pair);
}


/*
 * Add a post parameter. This can be called multiple times and the values will
 * be appended.
 * @param key the parameter name
 * @param value the value
 */
void HttpRequest::AddPostParameter(const string &key, const string &value) {
  map<string, string>::iterator iter = m_post_params.find(key);

  if (iter == m_post_params.end()) {
    std::pair<string, string> pair(key, value);
    m_post_params.insert(pair);
  } else {
    iter->second.append(value);
  }
}


/*
 * Process post data
 */
void HttpRequest::ProcessPostData(const char *data, size_t *data_size) {
  MHD_post_process(m_processor, data, *data_size);
}


/*
 * Return the value of the header sent with this request
 * @param key the name of the header
 * @returns the value of the header or empty string if it doesn't exist.
 */
const string HttpRequest::GetHeader(const string &key) const {
  map<string, string>::const_iterator iter = m_headers.find(key);

  if (iter == m_headers.end())
    return "";
  else
    return iter->second;
}


/*
 * Return the value of a url parameter
 * @param key the name of the parameter
 * @return the value of the parameter
 */
const string HttpRequest::GetParameter(const string &key) const {
  const char *value = MHD_lookup_connection_value(m_connection,
                                                  MHD_GET_ARGUMENT_KIND,
                                                  key.data());
  if (value)
    return string(value);
  else
    return string();
}


/*
 * Lookup a post parameter in this request
 * @param key the name of the parameter
 * @return the value of the parameter or the empty string if it doesn't exist
 */
const string HttpRequest::GetPostParameter(const string &key) const {
  map<string, string>::const_iterator iter = m_post_params.find(key);

  if (iter == m_post_params.end())
    return "";
  else
    return iter->second;
}


/*
 * Set the content-type header
 * @param type, the content type
 * @return true if the header was set correctly, false otherwise
 */
void HttpResponse::SetContentType(const string &type) {
  SetHeader(MHD_HTTP_HEADER_CONTENT_TYPE, type);
}


/**
 * Set the appropriate headers so this response isn't cached
 */
void HttpResponse::SetNoCache() {
  SetHeader("Cache-Control", "no-cache, must-revalidate");
}


/*
 * Set a header in the response
 * @param key the header name
 * @param value the header value
 * @return true if the header was set correctly, false otherwise
 */
void HttpResponse::SetHeader(const string &key, const string &value) {
  std::pair<string, string> pair(key, value);
  m_headers.insert(pair);
}


/*
 * Send the HTTP response
 * @returns true on success, false on error
 */
int HttpResponse::Send() {
  map<string, string>::const_iterator iter;
  struct MHD_Response *response = MHD_create_response_from_data(
      m_data.length(),
      static_cast<void*>(const_cast<char*>(m_data.data())),
      MHD_NO,
      MHD_YES);
  for (iter = m_headers.begin(); iter != m_headers.end(); ++iter)
    MHD_add_response_header(response,
                            iter->first.c_str(),
                            iter->second.c_str());
  int ret = MHD_queue_response(m_connection, m_status_code, response);
  MHD_destroy_response(response);
  return ret;
}


/*
 * Setup the HTTP server.
 * @param port the port to listen on
 * @param data_dir the directory to serve static content from
 */
HttpServer::HttpServer(unsigned int port, const string &data_dir)
    : OlaThread(),
      m_httpd(NULL),
      m_default_handler(NULL),
      m_port(port),
      m_data_dir(data_dir) {
  if (m_data_dir.empty())
    m_data_dir = HTTP_DATA_DIR;
}


/*
 * Destroy this object
 */
HttpServer::~HttpServer() {
  Stop();

  if (m_httpd)
    MHD_stop_daemon(m_httpd);

  map<string, BaseHttpCallback*>::const_iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter)
    delete iter->second;

  if (m_default_handler) {
    delete m_default_handler;
    m_default_handler = NULL;
  }

  m_handlers.clear();
}


/*
 * Setup the HTTP server
 * @return true on success, false on failure
 */
bool HttpServer::Init() {
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

  if (m_httpd)
    m_select_server.RunInLoop(NewCallback(this, &HttpServer::UpdateSockets));

  return m_httpd ? true : false;
}


/**
 * The entry point into the new thread
 */
void *HttpServer::Run() {
  if (!m_httpd) {
    OLA_WARN << "HttpServer::Run called but the server wasn't setup.";
    return NULL;
  }

  OLA_INFO << "HTTP Server started on port " << m_port;

  // set a long poll interval so we don't spin
  m_select_server.SetDefaultInterval(TimeInterval(60, 0));
  m_select_server.Run();

  // clean up any remaining sockets
  set<UnmanagedSocket*, unmanaged_socket_lt>::iterator iter =
    m_sockets.begin();
  for (; iter != m_sockets.end(); ++iter) {
    m_select_server.RemoveSocket(*iter);
    m_select_server.UnRegisterWriteSocket(*iter);
    delete *iter;
  }
  return NULL;
}


/*
 * Stop the HTTP server
 */
void HttpServer::Stop() {
  if (IsRunning()) {
    OLA_INFO << "Notifying HTTP server thread to stop";
    m_select_server.Terminate();
    OLA_INFO << "Waiting for HTTP server thread to exit";
    Join();
    OLA_INFO << "HTTP server thread exited";
  }
}


/**
 * This is run every loop iteration to update the list of sockets in the
  * SelectServer from MHD.
 */
void HttpServer::UpdateSockets() {
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

  if (MHD_YES != MHD_get_fdset(m_httpd, &r_set, &w_set, &e_set, &max_fd)) {
    OLA_WARN << "Failed to get a list of the file descriptors for MHD";
    return;
  }

  set<UnmanagedSocket*, unmanaged_socket_lt>::iterator iter =
    m_sockets.begin();

  // This isn't the best plan, talk to the MHD devs about exposing the list of
  // FD in a more suitable way
  int i = 0;
  while (iter != m_sockets.end() && i <= max_fd) {
    if ((*iter)->ReadDescriptor() < i) {
      // this socket is no longer required so remove it
      OLA_DEBUG << "Removing unsed socket " << (*iter)->ReadDescriptor();
      m_select_server.RemoveSocket(*iter);
      m_select_server.UnRegisterWriteSocket(*iter);
      delete *iter;
      m_sockets.erase(iter++);
    } else if ((*iter)->ReadDescriptor() == i) {
      // this socket may need to be updated
      if (FD_ISSET(i, &r_set))
        m_select_server.AddSocket(*iter);
      else
        m_select_server.RemoveSocket(*iter);

      if (FD_ISSET(i, &w_set))
        m_select_server.RegisterWriteSocket(*iter);
      else
        m_select_server.UnRegisterWriteSocket(*iter);
      iter++;
      i++;
    } else {
      // this is a new socket
      if (FD_ISSET(i, &r_set) || FD_ISSET(i, &w_set)) {
        OLA_DEBUG << "Adding new socket " << i;
        UnmanagedSocket *socket = NewSocket(&r_set, &w_set, i);
        m_sockets.insert(socket);
      }
      i++;
    }
  }

  while (iter != m_sockets.end()) {
    OLA_DEBUG << "Removing " << (*iter)->ReadDescriptor() <<
      " as it's not longer needed";
    m_select_server.UnRegisterWriteSocket(*iter);
    m_select_server.RemoveSocket(*iter);
    delete *iter;
    m_sockets.erase(iter++);
  }

  for (;i <= max_fd; i++) {
    // add the remaining sockets to the SS
    if (FD_ISSET(i, &r_set) || FD_ISSET(i, &w_set)) {
      OLA_DEBUG << "Adding " << i << " as a new socket";
      UnmanagedSocket *socket = NewSocket(&r_set, &w_set, i);
      m_sockets.insert(socket);
    }
  }
}


/**
 * Called when there is HTTP IO activity to deal with. This is a noop as
 * MHD_run is called in UpdateSockets above.
 */
void HttpServer::HandleHTTPIO() {}


/*
 * Call the appropriate handler.
 */
int HttpServer::DispatchRequest(const HttpRequest *request,
                                HttpResponse *response) {
  map<string, BaseHttpCallback*>::iterator iter =
    m_handlers.find(request->Url());

  if (iter != m_handlers.end())
    return iter->second->Run(request, response);

  map<string, static_file_info>::iterator file_iter =
    m_static_content.find(request->Url());

  if (file_iter != m_static_content.end())
    return ServeStaticContent(&(file_iter->second), response);

  if (m_default_handler)
    return m_default_handler->Run(request, response);

  return ServeNotFound(response);
}


/*
 * Register a handler
 * @param path the url to respond on
 * @param handler the Closure to call for this request. These will be freed
 * once the HttpServer is destroyed.
 */
bool HttpServer::RegisterHandler(const string &path,
                                 BaseHttpCallback *handler) {
  map<string, BaseHttpCallback*>::const_iterator iter = m_handlers.find(path);
  if (iter != m_handlers.end())
    return false;
  pair<string, BaseHttpCallback*> pair(path, handler);
  m_handlers.insert(pair);
  return true;
}


/*
 * Register a static file
 * @param path the path to serve on
 * @param file the path to the file to serve
 */
bool HttpServer::RegisterFile(const string &path,
                              const string &file,
                              const string &content_type) {
  map<string, static_file_info>::const_iterator file_iter = (
      m_static_content.find(path));

  if (file_iter != m_static_content.end())
    return false;

  static_file_info file_info;
  file_info.file_path = file;
  file_info.content_type = content_type;

  pair<string, static_file_info> pair(path, file_info);
  m_static_content.insert(pair);
  return true;
}


/*
 * Set the default handler.
 * @param handler the default handler to call. This will be freed when the
 * HttpServer is destroyed.
 */
void HttpServer::RegisterDefaultHandler(BaseHttpCallback *handler) {
  m_default_handler = handler;
}


/*
 * Return a list of all handlers registered
 */
vector<string> HttpServer::Handlers() const {
  vector<string> handlers;
  map<string, BaseHttpCallback*>::const_iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter)
    handlers.push_back(iter->first);

  map<string, static_file_info>::const_iterator file_iter;
  for (file_iter = m_static_content.begin();
       file_iter != m_static_content.end(); ++file_iter)
    handlers.push_back(file_iter->first);
  return handlers;
}


/*
 * Serve an error.
 * @param response the reponse to use.
 * @param details the error description
 */
int HttpServer::ServeError(HttpResponse *response, const string &details) {
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


/*
 * Serve a 404
 * @param response the response to use
 */
int HttpServer::ServeNotFound(HttpResponse *response) {
  response->SetStatus(MHD_HTTP_NOT_FOUND);
  response->SetContentType(CONTENT_TYPE_HTML);
  response->SetStatus(404);
  response->Append("<b>404 Not Found</b>");
  int r = response->Send();
  delete response;
  return r;
}


/*
 * Serve static content.
 * @param file_info details on the file to server
 * @param response the response to use
 */
int HttpServer::ServeStaticContent(static_file_info *file_info,
                                   HttpResponse *response) {
  char *data;
  unsigned int length;
  string file_path = m_data_dir;
  file_path.append("/");
  file_path.append(file_info->file_path);
  ifstream i_stream(file_path.data());

  if (!i_stream.is_open()) {
    OLA_WARN << "Missing file: " << file_path;
    return ServeNotFound(response);
  }

  i_stream.seekg(0, std::ios::end);
  length = i_stream.tellg();
  i_stream.seekg(0, std::ios::beg);

  data = static_cast<char*>(malloc(length * sizeof(char)));

  i_stream.read(data, length);
  i_stream.close();

  struct MHD_Response *mhd_response = MHD_create_response_from_data(
      length,
      static_cast<void*>(data),
      MHD_YES,
      MHD_NO);

  if (!file_info->content_type.empty())
    MHD_add_response_header(mhd_response,
                            MHD_HTTP_HEADER_CONTENT_TYPE,
                            file_info->content_type.data());

  int ret = MHD_queue_response(response->Connection(),
                               MHD_HTTP_OK,
                               mhd_response);
  MHD_destroy_response(mhd_response);
  delete response;
  return ret;
}


UnmanagedSocket *HttpServer::NewSocket(fd_set *r_set,
                                       fd_set *w_set,
                                       int fd) {
  UnmanagedSocket *socket = new UnmanagedSocket(fd);
  socket->SetOnData(NewCallback(this, &HttpServer::HandleHTTPIO));
  socket->SetOnWritable(NewCallback(this, &HttpServer::HandleHTTPIO));

  if (FD_ISSET(fd, r_set))
    m_select_server.AddSocket(socket);

  if (FD_ISSET(fd, w_set))
    m_select_server.RegisterWriteSocket(socket);
  return socket;
}
}  // ola
