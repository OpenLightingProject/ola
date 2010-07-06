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
 * InternalRDMController.cpp
 * Manager internally generated RDM requests
 * Copyright (C) 2010 Simon Newton
 */

#include <map>
#include <string>
#include <vector>
#include "olad/InternalRDMController.h"

namespace ola {

using ola::rdm::RDMCommand;

const char InternalRDMController::MISMATCHED_RDM_RESPONSE_VAR[] =
  "rdm-mismatched-responses";
const char InternalRDMController::EXPIRED_RDM_REQUESTS_VAR[] =
  "rdm-expired-requests";

/*
 * Create a new OutstandingRDMRequest
 */
OutstandingRDMRequest::OutstandingRDMRequest(
    const RDMRequest &request,
    rdm_controller_callback *callback):
      m_source_uid(request.SourceUID()),
      m_sub_device(request.SubDevice()),
      m_transaction_number(request.TransactionNumber()),
      m_command_class(request.CommandClass()),
      m_expires(),
      m_callback(callback) {
  Clock::CurrentTime(&m_expires);
}


/*
 * Returns true if this response matches this request
 */
bool OutstandingRDMRequest::Matches(const RDMResponse *response) {
  if (!response)
    return false;

  if (response->DestinationUID() == m_source_uid &&
      response->TransactionNumber() == m_transaction_number &&
      response->SubDevice() == m_sub_device) {
    if (response->CommandClass() == RDMCommand::GET_COMMAND_RESPONSE &&
        m_command_class == RDMCommand::GET_COMMAND)
      return true;
    if (response->CommandClass() == RDMCommand::SET_COMMAND_RESPONSE &&
        m_command_class == RDMCommand::SET_COMMAND)
      return true;
  }
  return false;
}


/*
 * Return true if this request has expired
 */
bool OutstandingRDMRequest::HasExpired(const TimeStamp &now) {
  return now > m_expires;
}


/*
 * Run the callback for this request
 */
void OutstandingRDMRequest::RunCallback(const RDMResponse *response) {
  if (m_callback)
    m_callback->Run(response);
  m_callback = NULL;
}


InternalRDMController::InternalRDMController(const UID &default_uid,
                                             PortManager *port_manager,
                                             class ExportMap *export_map):
    m_default_uid(default_uid),
    m_port_manager(port_manager),
    m_export_map(export_map) {

  m_export_map->GetIntegerVar(MISMATCHED_RDM_RESPONSE_VAR);
  m_export_map->GetIntegerVar(EXPIRED_RDM_REQUESTS_VAR);
}


/*
 * Tear down this InternalRDMController
 */
InternalRDMController::~InternalRDMController() {
  // delete all ports
  map<unsigned int, InternalInputPort*>::iterator port_iter;
  for (port_iter = m_input_ports.begin(); port_iter != m_input_ports.end();
       ++port_iter) {
    m_port_manager->UnPatchPort(port_iter->second);
    delete port_iter->second;
  }
  m_input_ports.clear();

  // delete out OutstandingRDMRequests
  map<unsigned int, vector<OutstandingRDMRequest*> >::iterator iter;
  vector<OutstandingRDMRequest*>::iterator request_iter;

  for (iter = m_outstanding_requests.begin();
       iter != m_outstanding_requests.end();) {
    for (request_iter = iter->second.begin();
         request_iter != iter->second.end();) {
      (*request_iter)->RunCallback(NULL);
      delete *request_iter;
    }
  }
  m_transaction_numbers.clear();
}


/*
 * Send an RDMRequest, this may run the callback immediately if the send failed
 */
bool InternalRDMController::SendRDMRequest(
    Universe *universe,
    const UID &destination,
    uint8_t sub_device,
    uint16_t param_id,
    const string &data,
    bool is_set,
    rdm_controller_callback *callback,
    const UID *source) {
  map<unsigned int, InternalInputPort*>::iterator port_iter =
    m_input_ports.find(universe->UniverseId());

  if (port_iter == m_input_ports.end()) {
    // create a new InternalInputPort for this universe
    InternalInputPort *input_port = new InternalInputPort(
      universe->UniverseId(),
      this);
    if (!m_port_manager->PatchPort(input_port, universe->UniverseId())) {
      OLA_WARN << "Failed to patch internal input port to universe " <<
      universe->UniverseId() << ", aborting RDM request";
      delete callback;
      return false;
    }
    port_iter = m_input_ports.insert(pair<unsigned int, InternalInputPort*>(
      universe->UniverseId(),
      input_port)).first;
  }

  UID source_uid = m_default_uid;
  if (source)
    source_uid = *source;

  // lookup transaction number
  uint8_t transaction_number = 0;
  pair<unsigned int, const UID> transaction_key(universe->UniverseId(),
                                                source_uid);
  map<pair<unsigned int, const UID>, uint8_t>::iterator transaction_iter =
    m_transaction_numbers.find(transaction_key);
  if (transaction_iter == m_transaction_numbers.end())
    m_transaction_numbers[transaction_key] = 0;
  else
    transaction_number = transaction_iter->second++;

  ola::rdm::RDMRequest *request = NULL;
  if (is_set) {
    request = new ola::rdm::RDMSetRequest(
      source_uid,
      destination,
      transaction_number,
      1,  // port id
      0,  // message count
      sub_device,
      param_id,
      reinterpret_cast<const uint8_t*>(data.data()),
      data.size());
  } else {
    request = new ola::rdm::RDMGetRequest(
      source_uid,
      destination,
      transaction_number,
      1,  // port id
      0,  // message count
      sub_device,
      param_id,
      reinterpret_cast<const uint8_t*>(data.data()),
      data.size());
  }
  if (port_iter->second->HandleRDMRequest(request)) {
    // add to queue here
    OutstandingRDMRequest *outstanding_request =
      new OutstandingRDMRequest(*request, callback);
    m_outstanding_requests[universe->UniverseId()].push_back(
      outstanding_request);
  } else {
    callback->Run(NULL);
  }
  return true;
}


/*
 * Handle RDM responses
 */
bool InternalRDMController::HandleRDMResponse(
    unsigned int universe,
    const ola::rdm::RDMResponse *response) {
  // try to locate a match
  OutstandingRDMRequest *request = NULL;
  map<unsigned int, vector<OutstandingRDMRequest*> >::iterator iter =
    m_outstanding_requests.find(universe);

  if (iter == m_outstanding_requests.end()) {
    // things have gone horibly wrong
    OLA_WARN << "Got a RDMResponse but can't find universe " << universe;
    delete response;
    return false;
  }

  vector<OutstandingRDMRequest*>::iterator request_iter;
  for (request_iter = iter->second.begin();
       request_iter != iter->second.end(); ++request_iter) {
    if ((*request_iter)->Matches(response)) {
      request = *request_iter;
      iter->second.erase(request_iter);
      break;
    }
  }

  if (!request) {
    OLA_WARN << "Unable to locate a matching request for RDM response";
    (*m_export_map->GetIntegerVar(MISMATCHED_RDM_RESPONSE_VAR))++;
    delete response;
    return false;
  }

  (*request_iter)->RunCallback(response);
  return true;
}


/*
 * Check for any exprired requests
 * @param now the current time
 */
void InternalRDMController::CheckTimeouts(const TimeStamp &now) {
  vector<OutstandingRDMRequest*> expired_requests;
  map<unsigned int, vector<OutstandingRDMRequest*> >::iterator iter;
  vector<OutstandingRDMRequest*>::iterator request_iter;

  for (iter = m_outstanding_requests.begin();
       iter != m_outstanding_requests.end(); ++iter) {
    for (request_iter = iter->second.begin();
         request_iter != iter->second.end();) {
      if ((*request_iter)->HasExpired(now)) {
        expired_requests.push_back(*request_iter);
        request_iter = iter->second.erase(request_iter);
      } else {
        request_iter++;
      }
    }
  }

  for (request_iter = expired_requests.begin();
       request_iter != expired_requests.end(); ++request_iter) {
    (*m_export_map->GetIntegerVar(EXPIRED_RDM_REQUESTS_VAR))++;
    (*request_iter)->RunCallback(NULL);
    delete *request_iter;
  }
  expired_requests.clear();
}
}  // ola
