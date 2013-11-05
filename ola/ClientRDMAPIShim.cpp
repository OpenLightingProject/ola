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
 * ClientRDMAPIShim.cpp
 * An implemention of RDMAPIImplInterface that uses the OlaClient.
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/client/ClientRDMAPIShim.h>
#include <ola/client/OlaClient.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMEnums.h>

#include <string>

namespace ola {
namespace client {

bool ClientRDMAPIShim::RDMGet(rdm_callback *callback,
                              unsigned int universe,
                              const ola::rdm::UID &uid,
                              uint16_t sub_device,
                              uint16_t pid,
                              const uint8_t *data,
                              unsigned int data_length) {
  SendRDMArgs args(NewSingleCallback(
      this, &ClientRDMAPIShim::HandleResponse, callback));
  m_client->RDMGet(universe, uid, sub_device, pid, data, data_length, args);
  return true;
}

bool ClientRDMAPIShim::RDMGet(rdm_pid_callback *callback,
                              unsigned int universe,
                              const ola::rdm::UID &uid,
                              uint16_t sub_device,
                              uint16_t pid,
                              const uint8_t *data,
                              unsigned int data_length) {
  SendRDMArgs args(NewSingleCallback(
      this, &ClientRDMAPIShim::HandleResponseWithPid, callback));
  m_client->RDMGet(universe, uid, sub_device, pid, data, data_length, args);
  return true;
}

bool ClientRDMAPIShim::RDMSet(rdm_callback *callback,
                              unsigned int universe,
                              const ola::rdm::UID &uid,
                              uint16_t sub_device,
                              uint16_t pid,
                              const uint8_t *data,
                              unsigned int data_length) {
  SendRDMArgs args(NewSingleCallback(
      this, &ClientRDMAPIShim::HandleResponse, callback));
  m_client->RDMSet(universe, uid, sub_device, pid, data, data_length, args);
  return true;
}

void ClientRDMAPIShim::HandleResponse(rdm_callback *callback,
                                      const Result &result,
                                      const RDMMetadata &metadata,
                                      const ola::rdm::RDMResponse *response) {
  rdm::ResponseStatus response_status;
  string data;
  GetResponseStatusAndData(result, metadata.response_code, response,
                           &response_status, &data);
  callback->Run(response_status, data);
}

void ClientRDMAPIShim::HandleResponseWithPid(
    rdm_pid_callback *callback,
    const Result &result,
    const RDMMetadata &metadata,
    const ola::rdm::RDMResponse *response) {
  rdm::ResponseStatus response_status;
  string data;
  GetResponseStatusAndData(result, metadata.response_code, response,
                           &response_status, &data);
  callback->Run(response_status, response_status.pid_value, data);
}

void ClientRDMAPIShim::GetResponseStatusAndData(
    const Result &result,
    ola::rdm::rdm_response_code response_code,
    const ola::rdm::RDMResponse *response,
    rdm::ResponseStatus *response_status,
    string *data) {
  response_status->error = result.Error();
  response_status->response_code = ola::rdm::RDM_FAILED_TO_SEND;

  if (result.Success()) {
    response_status->response_code = response_code;
    if (response_code == ola::rdm::RDM_COMPLETED_OK && response) {
      response_status->response_type = response->PortIdResponseType();
      response_status->message_count = response->MessageCount();
      response_status->pid_value = response->ParamId();
      response_status->set_command = (
          response->CommandClass() == ola::rdm::RDMCommand::SET_COMMAND_RESPONSE
           ? true : false);

      switch (response->PortIdResponseType()) {
        case ola::rdm::RDM_ACK:
          data->append(reinterpret_cast<const char*>(response->ParamData()),
                       response->ParamDataSize());
          break;
        case ola::rdm::RDM_ACK_TIMER:
          GetParamFromReply("ack timer", response, response_status);
          break;
        case ola::rdm::RDM_NACK_REASON:
          GetParamFromReply("nack", response, response_status);
          break;
        default:
          OLA_WARN << "Invalid response type 0x" << std::hex
                   << static_cast<int>(response->PortIdResponseType());
          response_status->response_type = ola::rdm::RDM_INVALID_RESPONSE;
      }
    }
  }
}

/**
 * Extract the uint16_t param for a ACK TIMER or NACK message and add it to the
 * ResponseStatus.
 */
void ClientRDMAPIShim::GetParamFromReply(const string &message_type,
                                         const ola::rdm::RDMResponse *response,
                                         ola::rdm::ResponseStatus *new_status) {
  uint16_t param;
  if (response->ParamDataSize() != sizeof(param)) {
    OLA_WARN << "Invalid PDL size for " << message_type << ", length was "
             << response->ParamDataSize();
    new_status->response_type = ola::rdm::RDM_INVALID_RESPONSE;
  } else {
    memcpy(&param, response->ParamData(), sizeof(param));
    new_status->m_param = ola::network::NetworkToHost(param);
  }
}
}  // namespace client
}  // namespace ola
