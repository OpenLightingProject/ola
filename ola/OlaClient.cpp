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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * OlaClient.cppFAIL
 * Implementation of OlaClient
 * Copyright (C) 2010 Simon Newton
 */

#include "ola/client/OlaClient.h"

#include <string>
#include <vector>

#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/OlaClientCore.h"
#include "ola/OlaDevice.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/RDMAPIImplInterface.h"
#include "ola/rdm/RDMEnums.h"

namespace ola {
namespace client {

using std::string;
using std::vector;
using ola::rdm::RDMAPIImplInterface;
using ola::io::ConnectedDescriptor;

OlaClient::OlaClient(ConnectedDescriptor *descriptor)
    : m_core(new OlaClientCore(descriptor)) {
}

OlaClient::~OlaClient() {
}

bool OlaClient::Setup() {
  return m_core->Setup();
}

bool OlaClient::Stop() {
  return m_core->Stop();
}

void OlaClient::SetCloseHandler(ola::SingleUseCallback0<void> *callback) {
  m_core->SetCloseHandler(callback);
}

void OlaClient::SetDMXCallback(RepeatableDMXCallback *callback) {
  m_core->SetDMXCallback(callback);
}

void OlaClient::ReloadPlugins(SetCallback *callback) {
  m_core->ReloadPlugins(callback);
}

void OlaClient::FetchPluginList(PluginListCallback *callback) {
  m_core->FetchPluginList(callback);
}

void OlaClient::FetchPluginDescription(ola_plugin_id plugin_id,
                                       PluginDescriptionCallback *callback) {
  m_core->FetchPluginDescription(plugin_id, callback);
}

void OlaClient::FetchPluginState(ola_plugin_id plugin_id,
                                 PluginStateCallback *callback) {
  m_core->FetchPluginState(plugin_id, callback);
}

void OlaClient::FetchDeviceInfo(ola_plugin_id plugin_filter,
                                DeviceInfoCallback *callback) {
  m_core->FetchDeviceInfo(plugin_filter, callback);
}

void OlaClient::FetchCandidatePorts(CandidatePortsCallback *callback) {
  m_core->FetchCandidatePorts(callback);
}

void OlaClient::FetchCandidatePorts(unsigned int universe_id,
                                            CandidatePortsCallback *callback) {
  m_core->FetchCandidatePorts(universe_id, callback);
}

void OlaClient::ConfigureDevice(unsigned int device_alias,
                                const string &msg,
                                ConfigureDeviceCallback *callback) {
  m_core->ConfigureDevice(device_alias, msg, callback);
}

void OlaClient::SetPluginState(ola_plugin_id plugin_id,
                               bool state,
                               SetCallback *callback) {
  m_core->SetPluginState(plugin_id, state, callback);
}

void OlaClient::SetPortPriorityInherit(unsigned int device_alias,
                                       unsigned int port,
                                       PortDirection port_direction,
                                       SetCallback *callback) {
  m_core->SetPortPriorityInherit(device_alias, port, port_direction, callback);
}

void OlaClient::SetPortPriorityOverride(unsigned int device_alias,
                                        unsigned int port,
                                        PortDirection port_direction,
                                        uint8_t value,
                                        SetCallback *callback) {
  m_core->SetPortPriorityOverride(device_alias, port, port_direction, value,
                                  callback);
}

void OlaClient::FetchUniverseList(UniverseListCallback *callback) {
  m_core->FetchUniverseList(callback);
}

void OlaClient::FetchUniverseInfo(unsigned int universe,
                                  UniverseInfoCallback *callback) {
  m_core->FetchUniverseInfo(universe, callback);
}

void OlaClient::SetUniverseName(unsigned int universe,
                                const string &name,
                                SetCallback *callback) {
  m_core->SetUniverseName(universe, name, callback);
}

void OlaClient::SetUniverseMergeMode(unsigned int universe,
                                     OlaUniverse::merge_mode mode,
                                     SetCallback *callback) {
  m_core->SetUniverseMergeMode(universe, mode, callback);
}

void OlaClient::Patch(unsigned int device_alias,
                      unsigned int port,
                      PortDirection port_direction,
                      PatchAction action,
                      unsigned int universe,
                      SetCallback *callback) {
  m_core->Patch(device_alias, port, port_direction, action, universe, callback);
}

void OlaClient::RegisterUniverse(unsigned int universe,
                                 RegisterAction register_action,
                                 SetCallback *callback) {
  m_core->RegisterUniverse(universe, register_action, callback);
}

void OlaClient::SendDMX(unsigned int universe,
                        const DmxBuffer &data,
                        const SendDMXArgs &args) {
  m_core->SendDMX(universe, data, args);
}

void OlaClient::FetchDMX(unsigned int universe, DMXCallback *callback) {
  m_core->FetchDMX(universe, callback);
}

void OlaClient::RunDiscovery(unsigned int universe,
                             DiscoveryType discovery_type,
                             DiscoveryCallback *callback) {
  m_core->RunDiscovery(universe, discovery_type, callback);
}

void OlaClient::SetSourceUID(const ola::rdm::UID &uid,
                             SetCallback *callback) {
  m_core->SetSourceUID(uid, callback);
}

void OlaClient::SendTimeCode(const ola::timecode::TimeCode &timecode,
                             SetCallback *callback) {
  m_core->SendTimeCode(timecode, callback);
}

void OlaClient::RDMGet(unsigned int universe,
                       const ola::rdm::UID &uid,
                       uint16_t sub_device,
                       uint16_t pid,
                       const uint8_t *data,
                       unsigned int data_length,
                       const SendRDMArgs& args) {
  m_core->RDMGet(universe, uid, sub_device, pid, data, data_length, args);
}

void OlaClient::RDMSet(unsigned int universe,
                       const ola::rdm::UID &uid,
                       uint16_t sub_device,
                       uint16_t pid,
                       const uint8_t *data,
                       unsigned int data_length,
                       const SendRDMArgs& args) {
  m_core->RDMSet(universe, uid, sub_device, pid, data, data_length, args);
}
}  // namespace client
}  // namespace ola
