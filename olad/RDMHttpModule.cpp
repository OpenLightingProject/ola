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
 * RDMHttpModule.cpp
 * This module acts as the http -> olad gateway for RDM commands.
 * Copyright (C) 2010 Simon Newton
 */

#include <iostream>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/rdm/RDMHelper.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "olad/RDMHttpModule.h"
#include "olad/OlaServer.h"
#include "ola/OlaCallbackClient.h"


namespace ola {

using ola::rdm::UID;
using std::endl;
using std::string;
using std::stringstream;
using std::vector;


const char RDMHttpModule::BACKEND_DISCONNECTED_ERROR[] =
    "Failed to send request, client isn't connected";


/**
 * Create a new OLA HTTP server
 * @param export_map the ExportMap to display when /debug is called
 * @param client_socket A ConnectedSocket which is used to communicate with the
 *   server.
 * @param
 */
RDMHttpModule::RDMHttpModule(HttpServer *http_server,
                             OlaCallbackClient *client)
    : m_server(http_server),
      m_client(client),
      m_rdm_api(m_client) {
  RegisterHandler("/json/rdm/supported_pids",
                  &RDMHttpModule::JsonSupportedPIDs);
}


/*
 * Teardown
 */
RDMHttpModule::~RDMHttpModule() {}



/**
 * Return a list of pids supported by this device.
 * @param request the HttpRequest
 * @param response the HttpResponse
 * @returns MHD_NO or MHD_YES
 */
int RDMHttpModule::JsonSupportedPIDs(const HttpRequest *request,
                                     HttpResponse *response) {
  string uni_id = request->GetParameter("id");
  string uid_string = request->GetParameter("uid");
  unsigned int universe_id;
  if (!StringToUInt(uni_id, &universe_id))
    return m_server->ServeNotFound(response);

  UID *uid = UID::FromString(uid_string);
  if (!uid)
    return m_server->ServeNotFound(response);

  string error;
  bool ok = m_rdm_api.GetSupportedParameters(
      universe_id,
      *uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this,
                        &RDMHttpModule::SupportedParamsHandler,
                        response),
      &error);
  delete uid;

  if (!ok)
    return m_server->ServeError(response, BACKEND_DISCONNECTED_ERROR);
  return MHD_YES;
}


/*
 * Register a handler
 */
inline void RDMHttpModule::RegisterHandler(
    const string &path,
    int (RDMHttpModule::*method)(const HttpRequest*, HttpResponse*)) {
  m_server->RegisterHandler(
      path,
      NewCallback<RDMHttpModule, int, const HttpRequest*, HttpResponse*>(
        this,
        method));
}


/*
 * Handle the response from a supported params request
 */
void RDMHttpModule::SupportedParamsHandler(
    HttpResponse *response,
    const ola::rdm::ResponseStatus &status,
    const vector<uint16_t> &pids) {
  stringstream str;
  if (CheckForRDMSuccess(status)) {
    vector<uint16_t>::const_iterator iter = pids.begin();

    str << "{" << endl;
    str << "  \"pids\": [" << endl;

    for (; iter != pids.end(); ++iter) {
      str << "    " << *iter << ",\n";
    }

    str << "  ]" << endl;
    str << "}";
  }

  response->SetContentType(HttpServer::CONTENT_TYPE_PLAIN);
  response->Append(str.str());
  response->Send();
  delete response;
}


/*
 * Check the success of an RDM command
 * @returns true if this command was ok, false otherwise.
 */
bool RDMHttpModule::CheckForRDMSuccess(
    const ola::rdm::ResponseStatus &status) {
  switch (status.ResponseType()) {
    case ola::rdm::ResponseStatus::TRANSPORT_ERROR:
      OLA_INFO << "RDM command error: " << status.Error();
      return false;
    case ola::rdm::ResponseStatus::BROADCAST_REQUEST:
      return false;
    case ola::rdm::ResponseStatus::REQUEST_NACKED:
      OLA_INFO << "Request was NACKED with code: " <<
        ola::rdm::NackReasonToString(status.NackReason());
      return false;
    case ola::rdm::ResponseStatus::MALFORMED_RESPONSE:
      OLA_INFO << "Malformed RDM response " << status.Error();
      return false;
    case ola::rdm::ResponseStatus::VALID_RESPONSE:
      return true;
    default:
      OLA_INFO << "Unknown response status " <<
        static_cast<int>(status.ResponseType());
      return false;
  }
}
}  // ola
