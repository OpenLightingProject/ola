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
 * Lla HTTP class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdio.h>

#include <fstream>
#include <iostream>
#include <string>
#include "HttpServer.h"

namespace lla {

using google::Template;
using google::TemplateDictionary;
using std::cout;
using std::endl;
using std::ifstream;
using std::pair;
using std::string;

const string HttpServer::CONTENT_TYPE_PLAIN = "text/plain";
const string HttpServer::CONTENT_TYPE_HTML = "text/html";
const string HttpServer::CONTENT_TYPE_GIF = "image/gif";
const string HttpServer::CONTENT_TYPE_PNG = "image/png";
const string HttpServer::CONTENT_TYPE_CSS = "text/css";
const string HttpServer::CONTENT_TYPE_JS = "text/javascript";

static int AddHeaders(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
  HttpRequest *request = (HttpRequest*) cls;
  string key_string = key;
  string value_string = value;
  request->AddHeader(key, value);
  return MHD_YES;
}

int IteratePost(void *request_cls, enum MHD_ValueKind kind, const char *key,
                const char *filename, const char *content_type,
                const char *transfer_encoding, const char *data, size_t off,
                size_t size) {
  HttpRequest *request = (HttpRequest*) request_cls;

  string value(data, size);
  cout << key << " : " << value << endl;
  request->AddPostParameter(key, value);
  return MHD_YES;
}


static int HandleRequest(void *http_server_ptr,
                         struct MHD_Connection *connection,
                         const char *url,
                         const char *method,
                         const char *version,
                         const char *upload_data,
                         unsigned int *upload_data_size,
                         void **ptr) {
  HttpServer *http_server = (HttpServer*) http_server_ptr;

  // first call
  if (*ptr == NULL) {
    HttpRequest *request = new HttpRequest(url, method, version,
                                           connection);
    if (!request)
      return MHD_NO;

    if (!request->Init()) {
      delete request;
      return MHD_NO;
    }
    *ptr = (void*) request;
    return MHD_YES;
  }

  HttpRequest *request = (HttpRequest*) *ptr;
  if (request->Method() == MHD_HTTP_METHOD_GET) {
    HttpResponse response(connection);
    return http_server->DispatchRequest(request, &response);
  } else if (request->Method() == MHD_HTTP_METHOD_POST) {
    cout << "post" << endl;
    if (*upload_data_size != 0) {
      request->ProcessPostData(upload_data, upload_data_size);
      *upload_data_size = 0;
      return MHD_YES;
    }
    HttpResponse response(connection);
    cout << "dispatch" << endl;
    return http_server->DispatchRequest(request, &response);
  }
  return MHD_NO;
}


void RequestCompleted(void *cls,
                      struct MHD_Connection *connection,
                      void **request_cls,
                      enum MHD_RequestTerminationCode toe) {
  HttpRequest *request = (HttpRequest*) *request_cls;

  if (!request)
    return;

  delete request;
  *request_cls = NULL;
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
  m_processor(NULL) {
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
                                            (void*) this);
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
 * Add a post parameter
 */
void HttpRequest::AddPostParameter(const string &key, const string &value) {
  std::pair<string, string> pair(key, value);
  m_post_params.insert(pair);
}


/*
 * Process post data
 */
void HttpRequest::ProcessPostData(const char *data,
                                  unsigned int *data_size) {

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
 *
 * @returns true on success, false on error
 */
int HttpResponse::Send() {
  map<string, string>::const_iterator iter;
  struct MHD_Response *response = MHD_create_response_from_data(
      m_data.length(), (void*) m_data.data(), MHD_NO, MHD_YES);
  for (iter = m_headers.begin(); iter != m_headers.end(); ++iter)
    MHD_add_response_header(response, iter->first.c_str(), iter->second.c_str());
  int ret = MHD_queue_response(m_connection, m_status_code, response);
  MHD_destroy_response(response);
  return ret;
}


/*
 * Setup the HTTP server.
 * @param port the port to listen on
 * @param data_dir the directory to serve static content from
 */
HttpServer::HttpServer(unsigned int port, const string &data_dir):
  m_httpd(NULL),
  m_default_handler(NULL),
  m_port(port),
  m_data_dir(data_dir) {

  if (m_data_dir.empty())
    m_data_dir = HTTP_DATA_DIR;

  google::Template::SetTemplateRootDirectory(m_data_dir);
}


/*
 * Destroy this object
 */
HttpServer::~HttpServer() {
  Stop();

  map<string, BaseHttpClosure*>::const_iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter)
    delete iter->second;

  if (m_default_handler) {
    delete m_default_handler;
    m_default_handler = NULL;
  }

  m_handlers.clear();
  google::Template::ClearCache();
}


/*
 * Start the HTTP server
 *
 * @return 0 on success, -1 on failure
 */
bool HttpServer::Start() {
  m_httpd = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
                             m_port,
                             NULL,
                             NULL,
                             &HandleRequest,
                             this,
                             MHD_OPTION_NOTIFY_COMPLETED,
                             RequestCompleted,
                             NULL,
                             MHD_OPTION_END);
  return m_httpd ? true : false;
}


/*
 * Stop the HTTP server
 */
void HttpServer::Stop() {
  if (m_httpd) {
    MHD_stop_daemon(m_httpd);
    m_httpd = NULL;
  }
}


/*
 * Call the appropriate handler.
 */
int HttpServer::DispatchRequest(const HttpRequest *request,
                                HttpResponse *response) {
  map<string, BaseHttpClosure*>::iterator iter = m_handlers.find(request->Url());

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
bool HttpServer::RegisterHandler(const string &path, BaseHttpClosure *handler) {
  map<string, BaseHttpClosure*>::const_iterator iter = m_handlers.find(path);
  if (iter != m_handlers.end())
    return false;
  pair<string, BaseHttpClosure*> pair(path, handler);
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
  map<string, static_file_info>::const_iterator file_iter = m_static_content.find(path);

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
void HttpServer::RegisterDefaultHandler(BaseHttpClosure *handler) {
  m_default_handler = handler;
}


/*
 * Return a list of all handlers registered
 */
vector<string> HttpServer::Handlers() const {
  vector<string> handlers;
  map<string, BaseHttpClosure*>::const_iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter)
    handlers.push_back(iter->first);

  map<string, static_file_info>::const_iterator file_iter;
  for (file_iter = m_static_content.begin(); file_iter != m_static_content.end();
       ++file_iter)
    handlers.push_back(file_iter->first);
  return handlers;
}


/*
 * Display a template
 * @param template_name the name of the template
 * @param dict the dictionary to expand with
 * @param response the response to use.
 */
int HttpServer::DisplayTemplate(const char *template_name,
                                TemplateDictionary *dict,
                                HttpResponse *response) {

  Template* tpl = Template::GetTemplate(template_name, google::STRIP_BLANK_LINES);

  if (!tpl)
    return ServeError(response, "Bad Template");

  string output;
  bool success = tpl->Expand(&output, dict);

  if (!success)
    return ServeError(response, "Expantion failed");

  response->SetContentType(HttpServer::CONTENT_TYPE_HTML);
  response->Append(output);
  return response->Send();
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
  return response->Send();
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
  return response->Send();
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

  if (!i_stream.is_open())
    return ServeNotFound(response);

  i_stream.seekg(0, std::ios::end);
  length = i_stream.tellg();
  i_stream.seekg(0, std::ios::beg);

  data = (char*) malloc(length * sizeof(char));

  i_stream.read(data, length);
  i_stream.close();

  struct MHD_Response *mhd_response = MHD_create_response_from_data(
      length,
      (void*) data,
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
  return ret;
}

} //lla
