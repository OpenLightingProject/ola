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
 * LlaHttpServer.h
 * Interface for the LLA HTTP class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_HTTP_SERVER_H
#define LLA_HTTP_SERVER_H

#include <string>
#include <lla/select_server/SelectServer.h>
#include <lla/ExportMap.h>
#include "HttpServer.h"

namespace lla {

using std::string;
using lla::select_server::SelectServer;

class LlaHttpServer: public HttpServer {
  public:
    LlaHttpServer(ExportMap *export_map,
                  SelectServer *ss,
                  unsigned int port,
                  bool enable_quit,
                  const string &data_dir);
    ~LlaHttpServer() {}

    int DisplayIndex(const HttpRequest *request, HttpResponse *response);
    int DisplayDebug(const HttpRequest *request, HttpResponse *response);
    int DisplayQuit(const HttpRequest *request, HttpResponse *response);
    int DisplayHandlers(const HttpRequest *request, HttpResponse *response);

  private :
    LlaHttpServer(const LlaHttpServer&);
    LlaHttpServer& operator=(const LlaHttpServer&);

    ExportMap *m_export_map;
    SelectServer *m_ss;
    bool m_enable_quit;

    static const string K_DATA_DIR_VAR;
};

} // lla
#endif
