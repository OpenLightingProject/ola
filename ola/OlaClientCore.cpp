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
 * OlaClientCore.cpp
 * Implementation of OlaClientCore
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "common/protocol/Ola.pb.h"
#include "ola/BaseTypes.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/OlaClientCore.h"
#include "ola/client/ClientTypes.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMEnums.h"

namespace ola {
namespace client {

using ola::proto::OlaServerService_Stub;
using std::auto_ptr;
using std::string;
using std::vector;
using ola::rdm::UID;
using ola::rdm::UIDSet;

const char OlaClientCore::NOT_CONNECTED_ERROR[] = "Not connected";

OlaClientCore::OlaClientCore(ConnectedDescriptor *descriptor)
    : m_descriptor(descriptor),
      m_connected(false) {
}


OlaClientCore::~OlaClientCore() {
  if (m_connected)
    Stop();
}


/*
 * Setup this client
 * @return true on success, false on failure
 */
bool OlaClientCore::Setup() {
  if (m_connected)
    return false;

  m_channel.reset(new RpcChannel(this, m_descriptor));

  if (!m_channel.get()) {
    return false;
  }
  m_stub.reset(new OlaServerService_Stub(m_channel.get()));

  if (!m_stub.get()) {
    m_channel.reset();
    return false;
  }
  m_connected = true;
  return true;
}


/*
 * Close the ola connection.
 * @return true on success, false on failure
 */
bool OlaClientCore::Stop() {
  if (m_connected) {
    m_descriptor->Close();
    m_channel.reset();
    m_stub.reset();
  }
  m_connected = false;
  return 0;
}

/**
 * Set the close handler.
 */
void OlaClientCore::SetCloseHandler(ola::SingleUseCallback0<void> *callback) {
  m_channel->SetChannelCloseHandler(callback);
}

void OlaClientCore::SetDMXCallback(RepeatableDMXCallback *callback) {
  m_dmx_callback.reset(callback);
}

void OlaClientCore::FetchPluginList(PluginListCallback *callback) {
  RpcController *controller = new RpcController();
  ola::proto::PluginListRequest request;
  ola::proto::PluginListReply *reply = new ola::proto::PluginListReply();

  if (m_connected) {
    CompletionCallback *cb = NewSingleCallback(
        this,
        &OlaClientCore::HandlePluginList,
        controller, reply, callback);
    m_stub->GetPlugins(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandlePluginList(controller, reply, callback);
  }
}

void OlaClientCore::FetchPluginDescription(
    ola_plugin_id plugin_id,
    PluginDescriptionCallback *callback) {
  RpcController *controller = new RpcController();
  ola::proto::PluginDescriptionRequest request;
  ola::proto::PluginDescriptionReply *reply = new
    ola::proto::PluginDescriptionReply();

  request.set_plugin_id(plugin_id);

  if (m_connected) {
    CompletionCallback *cb = NewSingleCallback(
        this,
        &OlaClientCore::HandlePluginDescription,
        controller, reply, callback);
    m_stub->GetPluginDescription(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandlePluginDescription(controller, reply, callback);
  }
}

void OlaClientCore::FetchPluginState(
    ola_plugin_id plugin_id,
    PluginStateCallback *callback) {
  RpcController *controller = new RpcController();
  ola::proto::PluginStateRequest request;
  ola::proto::PluginStateReply *reply = new ola::proto::PluginStateReply();

  request.set_plugin_id(plugin_id);

  if (m_connected) {
    CompletionCallback *cb = NewSingleCallback(
        this,
        &OlaClientCore::HandlePluginState,
        controller, reply, callback);
    m_stub->GetPluginState(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandlePluginState(controller, reply, callback);
  }
}

void OlaClientCore::FetchDeviceInfo(ola_plugin_id filter,
                                    DeviceInfoCallback *callback) {
  ola::proto::DeviceInfoRequest request;
  RpcController *controller = new RpcController();
  ola::proto::DeviceInfoReply *reply = new ola::proto::DeviceInfoReply();

  request.set_plugin_id(filter);

  if (m_connected) {
    CompletionCallback *cb = NewSingleCallback(
        this,
        &OlaClientCore::HandleDeviceInfo,
        controller, reply, callback);
    m_stub->GetDeviceInfo(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleDeviceInfo(controller, reply, callback);
  }
}

void OlaClientCore::FetchCandidatePorts(
    unsigned int universe_id,
    CandidatePortsCallback *callback) {
  GenericFetchCandidatePorts(universe_id, true, callback);
}

void OlaClientCore::FetchCandidatePorts(CandidatePortsCallback *callback) {
  GenericFetchCandidatePorts(0, false, callback);
}

void OlaClientCore::ConfigureDevice(
    unsigned int device_alias,
    const string &msg,
    ConfigureDeviceCallback *callback) {
  ola::proto::DeviceConfigRequest request;
  RpcController *controller = new RpcController();
  ola::proto::DeviceConfigReply *reply = new ola::proto::DeviceConfigReply();

  string configure_request;
  request.set_device_alias(device_alias);
  request.set_data(msg);

  if (m_connected) {
    CompletionCallback *cb = NewSingleCallback(
        this,
        &OlaClientCore::HandleDeviceConfig,
        controller, reply, callback);
    m_stub->ConfigureDevice(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleDeviceConfig(controller, reply, callback);
  }
}

void OlaClientCore::SetPortPriorityInherit(unsigned int device_alias,
                                           unsigned int port,
                                           PortDirection port_direction,
                                           SetCallback *callback) {
  ola::proto::PortPriorityRequest request;
  RpcController *controller = new RpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_device_alias(device_alias);
  request.set_port_id(port);
  request.set_is_output(port_direction == OUTPUT_PORT);
  request.set_priority_mode(ola::PRIORITY_MODE_INHERIT);

  if (m_connected) {
    CompletionCallback *cb = ola::NewSingleCallback(
        this,
        &OlaClientCore::HandleAck,
        controller, reply, callback);
    m_stub->SetPortPriority(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleAck(controller, reply, callback);
  }
}

void OlaClientCore::SetPortPriorityOverride(unsigned int device_alias,
                                            unsigned int port,
                                            PortDirection port_direction,
                                            uint8_t value,
                                            SetCallback *callback) {
  ola::proto::PortPriorityRequest request;
  RpcController *controller = new RpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_device_alias(device_alias);
  request.set_port_id(port);
  request.set_is_output(port_direction == OUTPUT_PORT);
  request.set_priority_mode(ola::PRIORITY_MODE_STATIC);
  request.set_priority(value);

  if (m_connected) {
    CompletionCallback *cb = ola::NewSingleCallback(
        this,
        &OlaClientCore::HandleAck,
        controller, reply, callback);
    m_stub->SetPortPriority(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleAck(controller, reply, callback);
  }
}

void OlaClientCore::FetchUniverseList(UniverseListCallback *callback) {
  RpcController *controller = new RpcController();
  ola::proto::OptionalUniverseRequest request;
  ola::proto::UniverseInfoReply *reply = new ola::proto::UniverseInfoReply();

  if (m_connected) {
    CompletionCallback *cb = ola::NewSingleCallback(
        this,
        &OlaClientCore::HandleUniverseList,
        controller, reply, callback);
    m_stub->GetUniverseInfo(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleUniverseList(controller, reply, callback);
  }
}

void OlaClientCore::FetchUniverseInfo(unsigned int universe_id,
                                      UniverseInfoCallback *callback) {
  RpcController *controller = new RpcController();
  ola::proto::OptionalUniverseRequest request;
  ola::proto::UniverseInfoReply *reply = new ola::proto::UniverseInfoReply();

  request.set_universe(universe_id);

  if (m_connected) {
    CompletionCallback *cb = ola::NewSingleCallback(
        this,
        &OlaClientCore::HandleUniverseInfo,
        controller, reply, callback);
    m_stub->GetUniverseInfo(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleUniverseInfo(controller, reply, callback);
  }
}

void OlaClientCore::SetUniverseName(unsigned int universe,
                                    const string &name,
                                    SetCallback *callback) {
  ola::proto::UniverseNameRequest request;
  RpcController *controller = new RpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_universe(universe);
  request.set_name(name);

  if (m_connected) {
    CompletionCallback *cb = ola::NewSingleCallback(
        this,
        &OlaClientCore::HandleAck,
        controller, reply, callback);
    m_stub->SetUniverseName(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleAck(controller, reply, callback);
  }
}

void OlaClientCore::SetUniverseMergeMode(unsigned int universe,
                                         OlaUniverse::merge_mode mode,
                                         SetCallback *callback) {
  ola::proto::MergeModeRequest request;
  RpcController *controller = new RpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  ola::proto::MergeMode merge_mode = mode == OlaUniverse::MERGE_HTP ?
    ola::proto::HTP : ola::proto::LTP;
  request.set_universe(universe);
  request.set_merge_mode(merge_mode);

  if (m_connected) {
    CompletionCallback *cb = ola::NewSingleCallback(
        this,
        &OlaClientCore::HandleAck,
        controller, reply, callback);
    m_stub->SetMergeMode(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleAck(controller, reply, callback);
  }
}

void OlaClientCore::Patch(unsigned int device_alias,
                          unsigned int port_id,
                          PortDirection port_direction,
                          PatchAction patch_action,
                          unsigned int universe,
                          SetCallback *callback) {
  ola::proto::PatchPortRequest request;
  RpcController *controller = new RpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  ola::proto::PatchAction action = (
      patch_action == PATCH ? ola::proto::PATCH : ola::proto::UNPATCH);
  request.set_universe(universe);
  request.set_device_alias(device_alias);
  request.set_port_id(port_id);
  request.set_is_output(port_direction == OUTPUT_PORT);
  request.set_action(action);

  if (m_connected) {
    CompletionCallback *cb = ola::NewSingleCallback(
        this,
        &OlaClientCore::HandleAck,
        controller, reply, callback);
    m_stub->PatchPort(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleAck(controller, reply, callback);
  }
}

void OlaClientCore::RegisterUniverse(unsigned int universe,
                                     RegisterAction register_action,
                                     SetCallback *callback) {
  ola::proto::RegisterDmxRequest request;
  RpcController *controller = new RpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  ola::proto::RegisterAction action = (
      register_action == REGISTER ? ola::proto::REGISTER :
        ola::proto::UNREGISTER);
  request.set_universe(universe);
  request.set_action(action);

  if (m_connected) {
    CompletionCallback *cb = ola::NewSingleCallback(
        this,
        &OlaClientCore::HandleAck,
        controller, reply, callback);
    m_stub->RegisterForDmx(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleAck(controller, reply, callback);
  }
}

void OlaClientCore::SendDMX(unsigned int universe,
                            const DmxBuffer &data,
                            const SendDMXArgs &args) {
  ola::proto::DmxData request;
  request.set_universe(universe);
  request.set_data(data.Get());
  request.set_priority(args.priority);

  if (args.callback) {
    // Full request
    RpcController *controller = new RpcController();
    ola::proto::Ack *reply = new ola::proto::Ack();

    if (m_connected) {
      CompletionCallback *cb = ola::NewSingleCallback(
          this,
          &OlaClientCore::HandleGeneralAck,
          controller, reply, args.callback);
      m_stub->UpdateDmxData(controller, &request, reply, cb);
     } else {
      controller->SetFailed(NOT_CONNECTED_ERROR);
      HandleGeneralAck(controller, reply, args.callback);
     }
  } else if (m_connected) {
    // stream data
    m_stub->StreamDmxData(NULL, &request, NULL, NULL);
  }
}

void OlaClientCore::FetchDMX(unsigned int universe,
                             DMXCallback *callback) {
  ola::proto::UniverseRequest request;
  RpcController *controller = new RpcController();
  ola::proto::DmxData *reply = new ola::proto::DmxData();

  request.set_universe(universe);

  if (m_connected) {
    CompletionCallback *cb = NewSingleCallback(
        this,
        &OlaClientCore::HandleGetDmx,
        controller, reply, callback);
    m_stub->GetDmx(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleGetDmx(controller, reply, callback);
  }
}

void OlaClientCore::RunDiscovery(unsigned int universe,
                                 DiscoveryType discovery_type,
                                 DiscoveryCallback *callback) {
  RpcController *controller = new RpcController();
  ola::proto::UIDListReply *reply = new ola::proto::UIDListReply();


  if (!m_connected) {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleUIDList(controller, reply, callback);
    return;
  }

  CompletionCallback *cb = NewSingleCallback(
      this,
      &OlaClientCore::HandleUIDList,
      controller, reply, callback);

  if (discovery_type == DISCOVERY_CACHED) {
    ola::proto::UniverseRequest request;
    request.set_universe(universe);
    m_stub->GetUIDs(controller, &request, reply, cb);
  } else {
    ola::proto::DiscoveryRequest request;
    request.set_universe(universe);
    request.set_full(discovery_type == DISCOVERY_FULL);
    m_stub->ForceDiscovery(controller, &request, reply, cb);
  }
}

void OlaClientCore::SetSourceUID(const UID &uid,
                                 SetCallback *callback) {
  ola::proto::UID request;
  RpcController *controller = new RpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_esta_id(uid.ManufacturerId());
  request.set_device_id(uid.DeviceId());

  if (m_connected) {
    CompletionCallback *cb = ola::NewSingleCallback(
        this,
        &OlaClientCore::HandleAck,
        controller, reply, callback);
    m_stub->SetSourceUID(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleAck(controller, reply, callback);
  }
}

void OlaClientCore::RDMGet(unsigned int universe,
                           const ola::rdm::UID &uid,
                           uint16_t sub_device,
                           uint16_t pid,
                           const uint8_t *data,
                           unsigned int data_length,
                           const SendRDMArgs& args) {
  SendRDMCommand(false, universe, uid, sub_device, pid, data, data_length,
                 args);
}

void OlaClientCore::RDMSet(unsigned int universe,
                           const ola::rdm::UID &uid,
                           uint16_t sub_device,
                           uint16_t pid,
                           const uint8_t *data,
                           unsigned int data_length,
                           const SendRDMArgs& args) {
  SendRDMCommand(true, universe, uid, sub_device, pid, data, data_length,
                 args);
}

void OlaClientCore::SendTimeCode(const ola::timecode::TimeCode &timecode,
                                 SetCallback *callback) {
  if (!timecode.IsValid()) {
    Result result("Invalid timecode");
    OLA_WARN << "Invalid timecode: " << timecode;
    if (callback) {
      callback->Run(result);
    }
    return;
  }

  RpcController *controller = new RpcController();
  ola::proto::TimeCode request;
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_type(static_cast<ola::proto::TimeCodeType>(timecode.Type()));
  request.set_hours(timecode.Hours());
  request.set_minutes(timecode.Minutes());
  request.set_seconds(timecode.Seconds());
  request.set_frames(timecode.Frames());

  if (m_connected) {
    CompletionCallback *cb = ola::NewSingleCallback(
        this,
        &OlaClientCore::HandleAck,
        controller, reply, callback);
    m_stub->SendTimeCode(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleAck(controller, reply, callback);
  }
}

void OlaClientCore::UpdateDmxData(ola::rpc::RpcController*,
                                  const ola::proto::DmxData *request,
                                  ola::proto::Ack*,
                                  CompletionCallback *done) {
  if (m_dmx_callback.get()) {
    DmxBuffer buffer;
    buffer.Set(request->data());

    uint8_t priority = 0;
    if (request->has_priority()) {
      priority = request->priority();
    }
    DMXMetadata metadata(request->universe(), priority);
    m_dmx_callback->Run(metadata, buffer);
  }
  done->Run();
}


// The following are RPC callbacks

/*
 * Called once PluginInfo completes
 */
void OlaClientCore::HandlePluginList(RpcController *controller_ptr,
                                     ola::proto::PluginListReply *reply_ptr,
                                     PluginListCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::PluginListReply> reply(reply_ptr);

  if (!callback) {
    return;
  }

  Result result(controller->Failed() ? controller->ErrorText() : "");

  vector<OlaPlugin> ola_plugins;
  if (!controller->Failed()) {
    for (int i = 0; i < reply->plugin_size(); ++i) {
      ola::proto::PluginInfo plugin_info = reply->plugin(i);
      OlaPlugin plugin(plugin_info.plugin_id(),
                       plugin_info.name(),
                       plugin_info.active());
      ola_plugins.push_back(plugin);
    }
  }
  std::sort(ola_plugins.begin(), ola_plugins.end());
  callback->Run(result, ola_plugins);
}


/*
 * Called once PluginState completes
 */
void OlaClientCore::HandlePluginDescription(
    RpcController *controller_ptr,
    ola::proto::PluginDescriptionReply *reply_ptr,
    PluginDescriptionCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::PluginDescriptionReply> reply(reply_ptr);

  if (!callback) {
    return;
  }

  Result result(controller->Failed() ? controller->ErrorText() : "");
  string description;

  if (!controller->Failed()) {
    description = reply->description();
  }
  callback->Run(result, description);
}


/*
 * Called once PluginState completes
 */
void OlaClientCore::HandlePluginState(
    RpcController *controller_ptr,
    ola::proto::PluginStateReply *reply_ptr,
    PluginStateCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::PluginStateReply> reply(reply_ptr);

  if (!callback) {
    return;
  }

  Result result(controller->Failed() ? controller->ErrorText() : "");
  PluginState plugin_state;

  if (!controller->Failed()) {
    plugin_state.name = reply->name();
    plugin_state.enabled = reply->enabled();
    plugin_state.active = reply->active();
    plugin_state.preferences_source = reply->preferences_source();
    for (int i = 0; i < reply->conflicts_with_size(); ++i) {
      ola::proto::PluginInfo plugin_info = reply->conflicts_with(i);
      OlaPlugin plugin(plugin_info.plugin_id(),
                       plugin_info.name(),
                       plugin_info.active());
      plugin_state.conflicting_plugins.push_back(plugin);
    }
  }

  callback->Run(result, plugin_state);
}


/*
 * Called once DeviceInfo completes.
 */
void OlaClientCore::HandleDeviceInfo(RpcController *controller_ptr,
                                     ola::proto::DeviceInfoReply *reply_ptr,
                                     DeviceInfoCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::DeviceInfoReply> reply(reply_ptr);

  if (!callback) {
    return;
  }

  Result result(controller->Failed() ? controller->ErrorText() : "");
  vector<OlaDevice> ola_devices;

  if (!controller->Failed()) {
    for (int i = 0; i < reply->device_size(); ++i) {
      ola::proto::DeviceInfo device_info = reply->device(i);
      vector<OlaInputPort> input_ports;

      for (int j = 0; j < device_info.input_port_size(); ++j) {
        ola::proto::PortInfo port_info = device_info.input_port(j);
        OlaInputPort port(
            port_info.port_id(),
            port_info.universe(),
            port_info.active(),
            port_info.description(),
            static_cast<port_priority_capability>(
              port_info.priority_capability()),
            static_cast<port_priority_mode>(
              port_info.priority_mode()),
            port_info.priority(),
            port_info.supports_rdm());
        input_ports.push_back(port);
      }

      vector<OlaOutputPort> output_ports;
      for (int j = 0; j < device_info.output_port_size(); ++j) {
        ola::proto::PortInfo port_info = device_info.output_port(j);
        OlaOutputPort port(
            port_info.port_id(),
            port_info.universe(),
            port_info.active(),
            port_info.description(),
            static_cast<port_priority_capability>(
              port_info.priority_capability()),
            static_cast<port_priority_mode>(
              port_info.priority_mode()),
            port_info.priority(),
            port_info.supports_rdm());
        output_ports.push_back(port);
      }

      OlaDevice device(device_info.device_id(),
                       device_info.device_alias(),
                       device_info.device_name(),
                       device_info.plugin_id(),
                       input_ports,
                       output_ports);
      ola_devices.push_back(device);
    }
  }
  std::sort(ola_devices.begin(), ola_devices.end());
  callback->Run(result, ola_devices);
}

void OlaClientCore::HandleDeviceConfig(RpcController *controller_ptr,
                                       ola::proto::DeviceConfigReply *reply_ptr,
                                       ConfigureDeviceCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::DeviceConfigReply> reply(reply_ptr);

  if (!callback) {
    return;
  }

  Result result(controller->Failed() ? controller->ErrorText() : "");
  string response_data;
  if (!controller->Failed()) {
    response_data = reply->data();
  }

  callback->Run(result, response_data);
}

void OlaClientCore::HandleAck(RpcController *controller_ptr,
                              ola::proto::Ack *reply_ptr,
                              SetCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::Ack> reply(reply_ptr);

  if (!callback) {
    return;
  }

  Result result(controller->Failed() ? controller->ErrorText() : "");
  callback->Run(result);
}

void OlaClientCore::HandleGeneralAck(RpcController *controller_ptr,
                                     ola::proto::Ack *reply_ptr,
                                     GeneralSetCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::Ack> reply(reply_ptr);

  if (!callback) {
    return;
  }

  Result result(controller->Failed() ? controller->ErrorText() : "");
  callback->Run(result);
}

void OlaClientCore::HandleUniverseList(RpcController *controller_ptr,
                                       ola::proto::UniverseInfoReply *reply_ptr,
                                       UniverseListCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::UniverseInfoReply> reply(reply_ptr);

  if (!callback) {
    return;
  }

  Result result(controller->Failed() ? controller->ErrorText() : "");
  vector<OlaUniverse> ola_universes;

  if (!controller->Failed()) {
    for (int i = 0; i < reply->universe_size(); ++i) {
      ola::proto::UniverseInfo universe_info = reply->universe(i);
      OlaUniverse::merge_mode merge_mode = (
        universe_info.merge_mode() == ola::proto::HTP ?
        OlaUniverse::MERGE_HTP: OlaUniverse::MERGE_LTP);

      OlaUniverse universe(universe_info.universe(),
                           merge_mode,
                           universe_info.name(),
                           universe_info.input_port_count(),
                           universe_info.output_port_count(),
                           universe_info.rdm_devices());
      ola_universes.push_back(universe);
    }
  }
  callback->Run(result, ola_universes);
}

void OlaClientCore::HandleUniverseInfo(RpcController *controller_ptr,
                                       ola::proto::UniverseInfoReply *reply_ptr,
                                       UniverseInfoCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::UniverseInfoReply> reply(reply_ptr);

  if (!callback) {
    return;
  }

  string error_str(controller->Failed() ? controller->ErrorText() : "");

  OlaUniverse null_universe(0, OlaUniverse::MERGE_LTP, "", 0, 0, 0);

  if (!controller->Failed()) {
    if (reply->universe_size() == 1) {
      ola::proto::UniverseInfo universe_info = reply->universe(0);
      OlaUniverse::merge_mode merge_mode = (
        universe_info.merge_mode() == ola::proto::HTP ?
        OlaUniverse::MERGE_HTP: OlaUniverse::MERGE_LTP);

      OlaUniverse universe(universe_info.universe(),
                           merge_mode,
                           universe_info.name(),
                           universe_info.input_port_count(),
                           universe_info.output_port_count(),
                           universe_info.rdm_devices());
      Result result(error_str);
      callback->Run(result, universe);
      return;
    } else if (reply->universe_size() > 1) {
      error_str = "Too many universes in response";
    } else {
      error_str = "Universe not found";
    }
  }
  Result result(error_str);
  callback->Run(result, null_universe);
}

void OlaClientCore::HandleGetDmx(RpcController *controller_ptr,
                                 ola::proto::DmxData *reply_ptr,
                                 DMXCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::DmxData> reply(reply_ptr);

  if (!callback) {
    return;
  }

  Result result(controller->Failed() ? controller->ErrorText() : "");
  DmxBuffer buffer;
  uint8_t priority = ola::dmx::SOURCE_PRIORITY_DEFAULT;

  if (!controller->Failed()) {
    buffer.Set(reply->data());
    priority = reply->priority();
  }
  DMXMetadata metadata(reply->universe(), priority);
  callback->Run(result, metadata, buffer);
}

void OlaClientCore::HandleUIDList(RpcController *controller_ptr,
                                  ola::proto::UIDListReply *reply_ptr,
                                  DiscoveryCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::UIDListReply> reply(reply_ptr);

  if (!callback) {
    return;
  }

  Result result(controller->Failed() ? controller->ErrorText() : "");
  UIDSet uids;

  if (!controller->Failed()) {
    for (int i = 0; i < reply->uid_size(); ++i) {
      const ola::proto::UID &proto_uid = reply->uid(i);
      ola::rdm::UID uid(proto_uid.esta_id(), proto_uid.device_id());
      uids.AddUID(uid);
    }
  }
  callback->Run(result, uids);
}

void OlaClientCore::HandleRDM(RpcController *controller_ptr,
                   ola::proto::RDMResponse *reply_ptr,
                   RDMCallback *callback) {
  auto_ptr<RpcController> controller(controller_ptr);
  auto_ptr<ola::proto::RDMResponse> reply(reply_ptr);

  if (!callback) {
    return;
  }

  Result result(controller->Failed() ? controller->ErrorText() : "");
  RDMMetadata metadata;
  ola::rdm::RDMResponse *response = NULL;

  if (!controller->Failed()) {
    response = BuildRDMResponse(reply.get(), &metadata.response_code);
  }
  callback->Run(result, metadata, response);
}

void OlaClientCore::GenericFetchCandidatePorts(
    unsigned int universe_id,
    bool include_universe,
    CandidatePortsCallback *callback) {
  ola::proto::OptionalUniverseRequest request;
  RpcController *controller = new RpcController();
  ola::proto::DeviceInfoReply *reply = new ola::proto::DeviceInfoReply();

  if (include_universe) {
    request.set_universe(universe_id);
  }

  if (m_connected) {
    CompletionCallback *cb = NewSingleCallback(
        this,
        &OlaClientCore::HandleDeviceInfo,
        controller, reply, callback);
    m_stub->GetCandidatePorts(controller, &request, reply, cb);
  } else {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleDeviceInfo(controller, reply, callback);
  }
}

/*
 * Send a generic rdm command
 */
void OlaClientCore::SendRDMCommand(bool is_set,
                                   unsigned int universe,
                                   const UID &uid,
                                   uint16_t sub_device,
                                   uint16_t pid,
                                   const uint8_t *data,
                                   unsigned int data_length,
                                   const SendRDMArgs &args) {
  if (!args.callback) {
    OLA_WARN << "RDM callback was null, command to " << uid << " won't be sent";
    return;
  }

  RpcController *controller = new RpcController();
  ola::proto::RDMResponse *reply = new ola::proto::RDMResponse();

  if (!m_connected) {
    controller->SetFailed(NOT_CONNECTED_ERROR);
    HandleRDM(controller, reply, args.callback);
    return;
  }

  ola::proto::RDMRequest request;
  request.set_universe(universe);
  ola::proto::UID *pb_uid = request.mutable_uid();
  pb_uid->set_esta_id(uid.ManufacturerId());
  pb_uid->set_device_id(uid.DeviceId());
  request.set_sub_device(sub_device);
  request.set_param_id(pid);
  request.set_is_set(is_set);
  request.set_data(string(reinterpret_cast<const char*>(data), data_length));

  CompletionCallback *cb = NewSingleCallback(
      this,
      &OlaClientCore::HandleRDM,
      controller, reply, args.callback);

  m_stub->RDMCommand(controller, &request, reply, cb);
}

/**
 * This constructs a ola::rdm::RDMResponse object from the information in a
 * ola::proto::RDMResponse.
 */
ola::rdm::RDMResponse *OlaClientCore::BuildRDMResponse(
    ola::proto::RDMResponse *reply,
    ola::rdm::rdm_response_code *response_code) {
  // Get the response code, if it's not RDM_COMPLETED_OK don't bother with the
  // rest of the response data.
  *response_code = static_cast<ola::rdm::rdm_response_code>(
      reply->response_code());
  if (*response_code != ola::rdm::RDM_COMPLETED_OK) {
    return NULL;
  }

  if (!reply->has_source_uid()) {
    OLA_WARN << "Missing source UID from RDMResponse";
    return NULL;
  }

  ola::rdm::UID source_uid(reply->source_uid().esta_id(),
                           reply->source_uid().device_id());

  if (!reply->has_dest_uid()) {
    OLA_WARN << "Missing dest UID from RDMResponse";
    return NULL;
  }

  ola::rdm::UID dest_uid(reply->dest_uid().esta_id(),
                         reply->dest_uid().device_id());

  if (!reply->has_transaction_number()) {
    OLA_WARN << "Missing transaction number from RDMResponse";
    return NULL;
  }

  if (!reply->has_command_class()) {
    OLA_WARN << "Missing command_class from RDMResponse";
    return NULL;
  }

  ola::rdm::RDMCommand::RDMCommandClass command_class =
      ola::rdm::RDMCommand::INVALID_COMMAND;
  switch (reply->command_class()) {
    case ola::proto::RDM_GET_RESPONSE :
      command_class = ola::rdm::RDMCommand::GET_COMMAND_RESPONSE;
      break;
    case ola::proto::RDM_SET_RESPONSE :
      command_class = ola::rdm::RDMCommand::SET_COMMAND_RESPONSE;
      break;
    default:
      OLA_WARN << "Unknown command class " << reply->command_class();
      return NULL;
  }

  return new ola::rdm::RDMResponse(
      source_uid, dest_uid, reply->transaction_number(),
      reply->response_type(), reply->message_count(),
      reply->sub_device(), command_class, reply->param_id(),
      reinterpret_cast<const uint8_t*>(reply->data().c_str()),
      reply->data().size());
}
}  // namespace client
}  // namespace ola
