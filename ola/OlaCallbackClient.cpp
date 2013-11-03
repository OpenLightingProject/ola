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
#include "ola/api/Result.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/RDMAPIImplInterface.h"
#include "ola/rdm/RDMEnums.h"

namespace ola {

using ola::api::Result;
using ola::io::ConnectedDescriptor;
using ola::rdm::RDMAPIImplInterface;
using std::string;

OlaCallbackClient::OlaCallbackClient(ConnectedDescriptor *descriptor)
    : m_core(new api::OlaClientCore(descriptor)) {
  m_core->SetDmxCallback(NewCallback(this, &OlaCallbackClient::HandleDMX));
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
      port_direction == INPUT_PORT ? api::INPUT_PORT: api::OUTPUT_PORT,
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
      port_direction == INPUT_PORT ? api::INPUT_PORT: api::OUTPUT_PORT,
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
      port_direction == INPUT_PORT ? api::INPUT_PORT: api::OUTPUT_PORT,
      action == PATCH ? api::PATCH : api::UNPATCH,
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
      register_action == REGISTER ? api::REGISTER : api::UNREGISTER,
      NewSingleCallback(this, &OlaCallbackClient::HandleSetCallback, callback));
  return true;
}

bool OlaCallbackClient::SendDmx(
    unsigned int universe,
    const DmxBuffer &data,
    SingleUseCallback1<void, const string&> *callback) {
  api::SendDmxArgs args(
      NewSingleCallback(this, &OlaCallbackClient::HandleSetCallback, callback));
  m_core->SendDmx(universe, args, data);
  return true;
}

bool OlaCallbackClient::SendDmx(
    unsigned int universe,
    const DmxBuffer &data,
    Callback1<void, const string&> *callback) {
  api::SendDmxArgs args(
      NewSingleCallback(this, &OlaCallbackClient::HandleRepeatableSetCallback,
        callback));
  m_core->SendDmx(universe, args, data);
  return true;
}

bool OlaCallbackClient::SendDmx(unsigned int universe, const DmxBuffer &data) {
  api::SendDmxArgs args;
  m_core->SendDmx(universe, args, data);
  return true;
}

bool OlaCallbackClient::FetchDmx(
    unsigned int universe,
    SingleUseCallback2<void, const DmxBuffer&, const string&> *callback) {
  m_core->FetchDmx(
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
      api::DISCOVERY_CACHED,
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
      universe, full ? api::DISCOVERY_FULL : api::DISCOVERY_INCREMENTAL,
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
  return m_core->RDMGet(callback, universe, uid, sub_device, pid, data,
                        data_length);
}

bool OlaCallbackClient::RDMGet(
    ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback,
    unsigned int universe,
    const ola::rdm::UID &uid,
    uint16_t sub_device,
    uint16_t pid,
    const uint8_t *data,
    unsigned int data_length) {
  return m_core->RDMGet(callback, universe, uid, sub_device, pid, data,
                        data_length);
}

bool OlaCallbackClient::RDMSet(
    ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
    unsigned int universe,
    const ola::rdm::UID &uid,
    uint16_t sub_device,
    uint16_t pid,
    const uint8_t *data,
    unsigned int data_length) {
  return m_core->RDMSet(callback, universe, uid, sub_device, pid, data,
                        data_length);
}

bool OlaCallbackClient::RDMSet(
    ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback,
    unsigned int universe,
    const ola::rdm::UID &uid,
    uint16_t sub_device,
    uint16_t pid,
    const uint8_t *data,
    unsigned int data_length) {
  return m_core->RDMSet(callback, universe, uid, sub_device, pid, data,
                        data_length);
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
    const api::Result &result,
    const std::vector<OlaPlugin> &plugins) {
  callback->Run(plugins, result.Error());
}

void OlaCallbackClient::HandlePluginDescription(
    SingleUseCallback2<void, const string&, const string&> *callback,
    const api::Result &result,
    const std::string &description) {
  callback->Run(description, result.Error());
}

void OlaCallbackClient::HandlePluginState(
    PluginStateCallback *callback,
    const api::Result &result,
    const api::PluginState &core_state) {
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
    const api::Result &result,
    const std::vector<OlaDevice> &devices) {
  callback->Run(devices, result.Error());
}

void OlaCallbackClient::HandleConfigureDevice(
    SingleUseCallback2<void, const string&, const string&> *callback,
    const api::Result &result,
    const std::string &reply) {
  callback->Run(reply, result.Error());
}

void OlaCallbackClient::HandleUniverseList(
    SingleUseCallback2<void, const std::vector<OlaUniverse>&,
                       const string &> *callback,
    const api::Result &result,
    const std::vector<OlaUniverse> &universes) {
  callback->Run(universes, result.Error());
}

void OlaCallbackClient::HandleUniverseInfo(
    SingleUseCallback2<void, OlaUniverse&, const string&> *callback,
    const api::Result &result,
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
    const api::Result &result,
    const api::DMXMetadata&,
    const DmxBuffer &data) {
  callback->Run(data, result.Error());
}

void OlaCallbackClient::HandleDiscovery(
    ola::SingleUseCallback2<void,
                            const ola::rdm::UIDSet&,
                            const string&> *callback,
    const api::Result &result,
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

void OlaCallbackClient::HandleDMX(const api::DMXMetadata &metadata,
                                  const DmxBuffer &data) {
  if (m_dmx_callback.get()) {
    m_dmx_callback->Run(metadata.universe, data, "");
  }

  if (m_dmx_callback.get()) {
    m_priority_dmx_callback->Run(metadata.universe, metadata.priority, data,
                                 "");
  }
}
}  // namespace ola
