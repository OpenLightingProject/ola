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
 * OlaCallbackClient.cpp
 * Implementation of OlaCallbackClient
 * Copyright (C) 2010 Simon Newton
 */

#include "ola/OlaCallbackClient.h"

#include <string>
#include <vector>

#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/OlaClientCore.h"
#include "ola/OlaDevice.h"
#include "ola/client/Result.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/RDMAPIImplInterface.h"
#include "ola/rdm/RDMEnums.h"

namespace ola {

using ola::client::RDMMetadata;
using ola::client::Result;
using ola::client::SendRDMArgs;
using ola::io::ConnectedDescriptor;
using ola::rdm::RDMAPIImplInterface;
using std::string;

OlaCallbackClient::OlaCallbackClient(ConnectedDescriptor *descriptor)
    : m_core(new client::OlaClientCore(descriptor)) {
  m_core->SetDMXCallback(NewCallback(this, &OlaCallbackClient::HandleDMX));
}

OlaCallbackClient::~OlaCallbackClient() {}

bool OlaCallbackClient::Setup() {
  return m_core->Setup();
}

bool OlaCallbackClient::Stop() {
  return m_core->Stop();
}

void OlaCallbackClient::SetCloseHandler(
    ola::SingleUseCallback0<void> *callback) {
  m_core->SetCloseHandler(callback);
}

bool OlaCallbackClient::FetchPluginList(
    SingleUseCallback2<void, const std::vector<OlaPlugin>&,
                       const string&> *callback) {
  m_core->FetchPluginList(
      NewSingleCallback(this, &OlaCallbackClient::HandlePluginList, callback));
  return true;
}

bool OlaCallbackClient::FetchPluginDescription(
    ola_plugin_id plugin_id,
    SingleUseCallback2<void, const string&, const string&> *callback) {
  m_core->FetchPluginDescription(
      plugin_id,
      NewSingleCallback(this, &OlaCallbackClient::HandlePluginDescription,
                        callback));
  return true;
}

bool OlaCallbackClient::FetchPluginState(ola_plugin_id plugin_id,
                                         PluginStateCallback *callback) {
  m_core->FetchPluginState(
      plugin_id,
      NewSingleCallback(this, &OlaCallbackClient::HandlePluginState,
                        callback));
  return true;
}

bool OlaCallbackClient::FetchDeviceInfo(
        ola_plugin_id filter,
        SingleUseCallback2<void,
                           const std::vector<OlaDevice>&,
                           const string&> *callback) {
  m_core->FetchDeviceInfo(
      filter,
      NewSingleCallback(this, &OlaCallbackClient::HandleDeviceInfo,
                        callback));
  return true;
}

bool OlaCallbackClient::FetchCandidatePorts(
        unsigned int universe_id,
        SingleUseCallback2<void,
                           const std::vector<OlaDevice>&,
                           const string&> *callback) {
  m_core->FetchCandidatePorts(
      universe_id,
      NewSingleCallback(this, &OlaCallbackClient::HandleDeviceInfo,
                        callback));
  return true;
}

bool OlaCallbackClient::FetchCandidatePorts(
        SingleUseCallback2<void,
                           const std::vector<OlaDevice>&,
                           const string&> *callback) {
  m_core->FetchCandidatePorts(
      NewSingleCallback(this, &OlaCallbackClient::HandleDeviceInfo,
                        callback));
  return true;
}

bool OlaCallbackClient::ConfigureDevice(
        unsigned int device_alias,
        const string &msg,
        SingleUseCallback2<void, const string&, const string&> *callback) {
  m_core->ConfigureDevice(
      device_alias, msg,
      NewSingleCallback(this, &OlaCallbackClient::HandleConfigureDevice,
                        callback));
  return true;
}

bool OlaCallbackClient::SetPortPriorityInherit(
    unsigned int device_alias,
    unsigned int port,
    PortDirection port_direction,
    SingleUseCallback1<void, const string&> *callback) {
  m_core->SetPortPriorityInherit(
      device_alias, port,
      port_direction == INPUT_PORT ? client::INPUT_PORT: client::OUTPUT_PORT,
      NewSingleCallback(this, &OlaCallbackClient::HandleSetCallback, callback));
  return true;
}

bool OlaCallbackClient::SetPortPriorityOverride(
    unsigned int device_alias,
    unsigned int port,
    PortDirection port_direction,
    uint8_t value,
    SingleUseCallback1<void, const string&> *callback) {
  m_core->SetPortPriorityOverride(
      device_alias, port,
      port_direction == INPUT_PORT ? client::INPUT_PORT: client::OUTPUT_PORT,
      value,
      NewSingleCallback(this, &OlaCallbackClient::HandleSetCallback, callback));
  return true;
}

bool OlaCallbackClient::FetchUniverseList(
    SingleUseCallback2<void, const std::vector<OlaUniverse>&, const string&>
        *callback) {
  m_core->FetchUniverseList(
      NewSingleCallback(this, &OlaCallbackClient::HandleUniverseList,
                        callback));
  return true;
}

bool OlaCallbackClient::FetchUniverseInfo(
    unsigned int universe,
    SingleUseCallback2<void, OlaUniverse&, const string&> *callback) {
  m_core->FetchUniverseInfo(
      universe,
      NewSingleCallback(this, &OlaCallbackClient::HandleUniverseInfo,
                        callback));
  return true;
}

bool OlaCallbackClient::SetUniverseName(
    unsigned int uni,
    const string &name,
    SingleUseCallback1<void, const string&> *callback) {
  m_core->SetUniverseName(
      uni, name,
      NewSingleCallback(this, &OlaCallbackClient::HandleSetCallback, callback));
  return true;
}

bool OlaCallbackClient::SetUniverseMergeMode(
    unsigned int uni,
    OlaUniverse::merge_mode mode,
    SingleUseCallback1<void, const string&> *callback) {
  m_core->SetUniverseMergeMode(
      uni, mode,
      NewSingleCallback(this, &OlaCallbackClient::HandleSetCallback, callback));
  return true;
}

bool OlaCallbackClient::Patch(
    unsigned int device_alias,
    unsigned int port,
    ola::PortDirection port_direction,
    ola::PatchAction action,
    unsigned int universe,
    SingleUseCallback1<void, const string&> *callback) {
  m_core->Patch(
      device_alias, port,
      port_direction == INPUT_PORT ? client::INPUT_PORT: client::OUTPUT_PORT,
      action == PATCH ? client::PATCH : client::UNPATCH,
      universe,
      NewSingleCallback(this, &OlaCallbackClient::HandleSetCallback, callback));
  return true;
}

void OlaCallbackClient::SetDmxCallback(
    Callback3<void, unsigned int, const DmxBuffer&, const string&> *callback) {
  m_dmx_callback.reset(callback);
}

void OlaCallbackClient::SetDmxCallback(
    Callback4<void, unsigned int, uint8_t, const DmxBuffer&, const string&>
        *callback) {
  m_priority_dmx_callback.reset(callback);
}

bool OlaCallbackClient::RegisterUniverse(
    unsigned int universe,
    ola::RegisterAction register_action,
    SingleUseCallback1<void, const string&> *callback) {
  m_core->RegisterUniverse(
      universe,
      register_action == REGISTER ? client::REGISTER : client::UNREGISTER,
      NewSingleCallback(this, &OlaCallbackClient::HandleSetCallback, callback));
  return true;
}

bool OlaCallbackClient::SendDmx(
    unsigned int universe,
    const DmxBuffer &data,
    SingleUseCallback1<void, const string&> *callback) {
  client::SendDMXArgs args(
      NewSingleCallback(this, &OlaCallbackClient::HandleSetCallback, callback));
  m_core->SendDMX(universe, data, args);
  return true;
}

bool OlaCallbackClient::SendDmx(
    unsigned int universe,
    const DmxBuffer &data,
    Callback1<void, const string&> *callback) {
  client::SendDMXArgs args(
      NewSingleCallback(this, &OlaCallbackClient::HandleRepeatableSetCallback,
        callback));
  m_core->SendDMX(universe, data, args);
  return true;
}

bool OlaCallbackClient::SendDmx(unsigned int universe, const DmxBuffer &data) {
  client::SendDMXArgs args;
  m_core->SendDMX(universe, data, args);
  return true;
}

bool OlaCallbackClient::FetchDmx(
    unsigned int universe,
    SingleUseCallback2<void, const DmxBuffer&, const string&> *callback) {
  m_core->FetchDMX(
      universe,
      NewSingleCallback(this, &OlaCallbackClient::HandleFetchDmx, callback));
  return true;
}

bool OlaCallbackClient::FetchUIDList(
        unsigned int universe,
        SingleUseCallback2<void,
                           const ola::rdm::UIDSet&,
                           const string&> *callback) {
  m_core->RunDiscovery(
      universe,
      client::DISCOVERY_CACHED,
      NewSingleCallback(this, &OlaCallbackClient::HandleDiscovery, callback));
  return true;
}

bool OlaCallbackClient::RunDiscovery(
        unsigned int universe,
        bool full,
        ola::SingleUseCallback2<void,
                                const ola::rdm::UIDSet&,
                                const string&> *callback) {
  m_core->RunDiscovery(
      universe, full ? client::DISCOVERY_FULL : client::DISCOVERY_INCREMENTAL,
      NewSingleCallback(this, &OlaCallbackClient::HandleDiscovery, callback));
  return true;
}

bool OlaCallbackClient::SetSourceUID(
    const ola::rdm::UID &uid,
    ola::SingleUseCallback1<void, const string&> *callback) {
  m_core->SetSourceUID(
      uid,
      NewSingleCallback(this, &OlaCallbackClient::HandleSetCallback, callback));
  return true;
}

bool OlaCallbackClient::RDMGet(
    ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
    unsigned int universe,
    const ola::rdm::UID &uid,
    uint16_t sub_device,
    uint16_t pid,
    const uint8_t *data,
    unsigned int data_length) {
  SendRDMArgs args(NewSingleCallback(
      this, &OlaCallbackClient::HandleRDMResponse, callback));
  m_core->RDMGet(universe, uid, sub_device, pid, data, data_length, args);
  return true;
}

bool OlaCallbackClient::RDMGet(
    ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback,
    unsigned int universe,
    const ola::rdm::UID &uid,
    uint16_t sub_device,
    uint16_t pid,
    const uint8_t *data,
    unsigned int data_length) {
  SendRDMArgs args(NewSingleCallback(
      this, &OlaCallbackClient::HandleRDMResponseWithPid, callback));
  m_core->RDMGet(universe, uid, sub_device, pid, data, data_length, args);
  return true;
}

bool OlaCallbackClient::RDMSet(
    ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
    unsigned int universe,
    const ola::rdm::UID &uid,
    uint16_t sub_device,
    uint16_t pid,
    const uint8_t *data,
    unsigned int data_length) {
  SendRDMArgs args(NewSingleCallback(
      this, &OlaCallbackClient::HandleRDMResponse, callback));
  m_core->RDMSet(universe, uid, sub_device, pid, data, data_length, args);
  return true;
}

bool OlaCallbackClient::RDMSet(
    ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback,
    unsigned int universe,
    const ola::rdm::UID &uid,
    uint16_t sub_device,
    uint16_t pid,
    const uint8_t *data,
    unsigned int data_length) {
  SendRDMArgs args(NewSingleCallback(
      this, &OlaCallbackClient::HandleRDMResponseWithPid, callback));
  m_core->RDMSet(universe, uid, sub_device, pid, data, data_length, args);
  return true;
}

bool OlaCallbackClient::SendTimeCode(
    ola::SingleUseCallback1<void, const string&> *callback,
    const ola::timecode::TimeCode &timecode) {
  m_core->SendTimeCode(
      timecode,
      NewSingleCallback(this, &OlaCallbackClient::HandleSetCallback, callback));
  return true;
}

// Handlers
// -------------------------------------------------------

void OlaCallbackClient::HandlePluginList(
    SingleUseCallback2<void, const std::vector<OlaPlugin>&,
                       const string&> *callback,
    const client::Result &result,
    const std::vector<OlaPlugin> &plugins) {
  callback->Run(plugins, result.Error());
}

void OlaCallbackClient::HandlePluginDescription(
    SingleUseCallback2<void, const string&, const string&> *callback,
    const client::Result &result,
    const std::string &description) {
  callback->Run(description, result.Error());
}

void OlaCallbackClient::HandlePluginState(
    PluginStateCallback *callback,
    const client::Result &result,
    const client::PluginState &core_state) {
  PluginState state;
  state.name = core_state.name;
  state.enabled = core_state.enabled;
  state.active = core_state.active;
  state.preferences_source = core_state.preferences_source;
  state.conflicting_plugins = core_state.conflicting_plugins;

  callback->Run(state, result.Error());
}

void OlaCallbackClient::HandleDeviceInfo(
    SingleUseCallback2<void, const std::vector<OlaDevice>&, const string&>
        *callback,
    const client::Result &result,
    const std::vector<OlaDevice> &devices) {
  callback->Run(devices, result.Error());
}

void OlaCallbackClient::HandleConfigureDevice(
    SingleUseCallback2<void, const string&, const string&> *callback,
    const client::Result &result,
    const std::string &reply) {
  callback->Run(reply, result.Error());
}

void OlaCallbackClient::HandleUniverseList(
    SingleUseCallback2<void, const std::vector<OlaUniverse>&,
                       const string &> *callback,
    const client::Result &result,
    const std::vector<OlaUniverse> &universes) {
  callback->Run(universes, result.Error());
}

void OlaCallbackClient::HandleUniverseInfo(
    SingleUseCallback2<void, OlaUniverse&, const string&> *callback,
    const client::Result &result,
    const OlaUniverse &universe) {
  // There was a bug in the API and universe isn't const.
  OlaUniverse new_universe(
      universe.Id(),
      universe.MergeMode(),
      universe.Name(),
      universe.InputPortCount(),
      universe.OutputPortCount(),
      universe.RDMDeviceCount());
  callback->Run(new_universe, result.Error());
}

void OlaCallbackClient::HandleFetchDmx(
    SingleUseCallback2<void, const DmxBuffer&, const string&> *callback,
    const client::Result &result,
    const client::DMXMetadata&,
    const DmxBuffer &data) {
  callback->Run(data, result.Error());
}

void OlaCallbackClient::HandleDiscovery(
    ola::SingleUseCallback2<void,
                            const ola::rdm::UIDSet&,
                            const string&> *callback,
    const client::Result &result,
    const ola::rdm::UIDSet &uids) {
  callback->Run(uids, result.Error());
}

void OlaCallbackClient::HandleSetCallback(
    ola::SingleUseCallback1<void, const string&> *callback,
    const Result &result) {
  callback->Run(result.Error());
}

void OlaCallbackClient::HandleRepeatableSetCallback(
    ola::Callback1<void, const string&> *callback,
    const Result &result) {
  callback->Run(result.Error());
}

void OlaCallbackClient::HandleDMX(const client::DMXMetadata &metadata,
                                  const DmxBuffer &data) {
  if (m_dmx_callback.get()) {
    m_dmx_callback->Run(metadata.universe, data, "");
  }

  if (m_priority_dmx_callback.get()) {
    m_priority_dmx_callback->Run(metadata.universe, metadata.priority, data,
                                 "");
  }
}

void OlaCallbackClient::HandleRDMResponse(
    ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
    const Result &result,
    const RDMMetadata &metadata,
    const ola::rdm::RDMResponse *response) {
  rdm::ResponseStatus response_status;
  string data;
  GetResponseStatusAndData(result, metadata.response_code, response,
                           &response_status, &data);
  callback->Run(response_status, data);
}

void OlaCallbackClient::HandleRDMResponseWithPid(
    ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback,
    const Result &result,
    const RDMMetadata &metadata,
    const ola::rdm::RDMResponse *response) {
  rdm::ResponseStatus response_status;
  string data;
  GetResponseStatusAndData(result, metadata.response_code, response,
                           &response_status, &data);
  callback->Run(response_status, response_status.pid_value, data);
}


void OlaCallbackClient::GetResponseStatusAndData(
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

void OlaCallbackClient::GetParamFromReply(
    const string &message_type,
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
}  // namespace ola
