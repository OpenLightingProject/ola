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
 * RDMHttpModule.h
 * This module acts as the http -> olad gateway for RDM commands.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef OLAD_RDMHTTPMODULE_H_
#define OLAD_RDMHTTPMODULE_H_

#include <string>
#include <vector>
#include "ola/Callback.h"
#include "ola/rdm/RDMAPI.h"
#include "olad/HttpServer.h"

namespace ola {

using std::string;


/*
 * The module that deals with RDM requests.
 * TODO(simon): factor out the common functionality like RegisterHandler into a
 *   parent class.
 */
class RDMHttpModule {
  public:
    RDMHttpModule(HttpServer *http_server,
                  class OlaCallbackClient *client);
    ~RDMHttpModule();

    int JsonSupportedPIDs(const HttpRequest *request, HttpResponse *response);

  private:
    HttpServer *m_server;
    class OlaCallbackClient *m_client;
    ola::rdm::RDMAPI m_rdm_api;

    RDMHttpModule(const RDMHttpModule&);
    RDMHttpModule& operator=(const RDMHttpModule&);

    void RegisterHandler(const string &path,
                         int (RDMHttpModule::*method)(const HttpRequest*,
                         HttpResponse*));

    void SupportedParamsHandler(HttpResponse *response,
                                const ola::rdm::ResponseStatus &status,
                                const vector<uint16_t> &pids);

    bool CheckForRDMSuccess(const ola::rdm::ResponseStatus &status);

    static const char BACKEND_DISCONNECTED_ERROR[];
};
}  // ola
#endif  // OLAD_RDMHTTPMODULE_H_
