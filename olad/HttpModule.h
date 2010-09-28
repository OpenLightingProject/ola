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
 * HttpModule.h
 * The base class for HTTP Modules which are used to extend the web UI
 * functionaiity.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef OLAD_HTTPMODULE_H_
#define OLAD_HTTPMODULE_H_

#include "ola/OlaCallbackClient.h"
#include "olad/HttpServer.h"

namespace ola {


/*
 * The base class that other modules inherit from.
 */
class HttpModule {
  public:
    HttpModule(HttpServer *http_server, class OlaCallbackClient *client);
    virtual ~HttpModule() {}

  protected:
    HttpServer *m_server;
    class OlaCallbackClient *m_client;

    HttpModule(const HttpModule&);
    HttpModule& operator=(const HttpModule&);

    static const char BACKEND_DISCONNECTED_ERROR[];
};
}  // ola
#endif  // OLAD_HTTPMODULE_H_
