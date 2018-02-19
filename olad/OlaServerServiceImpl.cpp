/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * OlaServerServiceImpl.cpp
 * Implementation of the OlaServerService interface. This is the class that
 * handles all the RPCs on the server side.
 * Copyright (C) 2005 Simon Newton
 */

#include <algorithm>
#include <string>
#include <vector>
#include "common/protocol/Ola.pb.h"
#include "common/rpc/RpcSession.h"
#include "ola/Callback.h"
#include "ola/CallbackRunner.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UIDSet.h"
#include "ola/strings/Format.h"
#include "ola/timecode/TimeCode.h"
#include "ola/timecode/TimeCodeEnums.h"
#include "olad/ClientBroker.h"
#include "olad/Device.h"
#include "olad/DmxSource.h"
#include "olad/OlaServerServiceImpl.h"
#include "olad/Plugin.h"
#include "olad/PluginManager.h"
#include "olad/Port.h"
#include "olad/Universe.h"
#include "olad/plugin_api/Client.h"
#include "olad/plugin_api/DeviceManager.h"
#include "olad/plugin_api/PortManager.h"
#include "olad/plugin_api/UniverseStore.h"

namespace ola {

using ola::CallbackRunner;
using ola::proto::Ack;
using ola::proto::DeviceConfigReply;
using ola::proto::DeviceConfigRequest;
using ola::proto::DeviceInfo;
using ola::proto::DeviceInfoReply;
using ola::proto::DeviceInfoRequest;
using ola::proto::DmxData;
using ola::proto::MergeModeRequest;
using ola::proto::OptionalUniverseRequest;
using ola::proto::PatchPortRequest;
using ola::proto::PluginDescriptionReply;
using ola::proto::PluginDescriptionRequest;
using ola::proto::PluginInfo;
using ola::proto::PluginListReply;
using ola::proto::PluginListRequest;
using ola::proto::PortInfo;
using ola::proto::RegisterDmxRequest;
using ola::proto::UniverseInfo;
using ola::proto::UniverseInfoReply;
using ola::proto::UniverseNameRequest;
using ola::proto::UniverseRequest;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::rpc::RpcController;
using std::string;
using std::vector;

namespace {

template<typename RequestType>

RDMRequest::OverrideOptions RDMRequestOptionsFromProto(
    const RequestType &request) {
  RDMRequest::OverrideOptions options;

  if (!request.has_options()) {
    return options;
  }

  const ola::proto::RDMRequestOverrideOptions &proto_options =
      request.options();
  if (proto_options.has_sub_start_code()) {
    options.sub_start_code = proto_options.sub_start_code();
  }
  if (proto_options.has_message_length()) {
    options.SetMessageLength(proto_options.message_length());
  }
  if (proto_options.has_message_count()) {
    options.message_count = proto_options.message_count();
  }
  if (proto_options.has_checksum()) {
    options.SetChecksum(proto_options.checksum());
  }
  return options;
}
}  // namespace

typedef CallbackRunner<ola::rpc::RpcService::CompletionCallback> ClosureRunner;

OlaServerServiceImpl::OlaServerServiceImpl(
    UniverseStore *universe_store,
    DeviceManager *device_manager,
    PluginManager *plugin_manager,
    PortManager *port_manager,
    ClientBroker *broker,
    const TimeStamp *wake_up_time,
    ReloadPluginsCallback *reload_plugins_callback)
    : m_universe_store(universe_store),
      m_device_manager(device_manager),
      m_plugin_manager(plugin_manager),
      m_port_manager(port_manager),
      m_broker(broker),
      m_wake_up_time(wake_up_time),
      m_reload_plugins_callback(reload_plugins_callback) {
}

void OlaServerServiceImpl::GetDmx(
    RpcController* controller,
    const UniverseRequest* request,
    DmxData* response,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe) {
    return MissingUniverseError(controller);
  }

  const DmxBuffer buffer = universe->GetDMX();
  response->set_data(buffer.Get());
  response->set_universe(request->universe());
}

void OlaServerServiceImpl::RegisterForDmx(
    RpcController* controller,
    const RegisterDmxRequest* request,
    Ack*,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverseOrCreate(
      request->universe());
  if (!universe) {
    return MissingUniverseError(controller);
  }

  Client *client = GetClient(controller);
  if (request->action() == ola::proto::REGISTER) {
    universe->AddSinkClient(client);
  } else {
    universe->RemoveSinkClient(client);
  }
}

void OlaServerServiceImpl::UpdateDmxData(
    RpcController* controller,
    const DmxData* request,
    Ack*,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe) {
    return MissingUniverseError(controller);
  }

  Client *client = GetClient(controller);
  DmxBuffer buffer;
  buffer.Set(request->data());

  uint8_t priority = ola::dmx::SOURCE_PRIORITY_DEFAULT;
  if (request->has_priority()) {
    priority = request->priority();
    priority = std::max(static_cast<uint8_t>(ola::dmx::SOURCE_PRIORITY_MIN),
                        priority);
    priority = std::min(static_cast<uint8_t>(ola::dmx::SOURCE_PRIORITY_MAX),
                        priority);
  }
  DmxSource source(buffer, *m_wake_up_time, priority);
  client->DMXReceived(request->universe(), source);
  universe->SourceClientDataChanged(client);
}

void OlaServerServiceImpl::StreamDmxData(
    RpcController *controller,
    const ola::proto::DmxData* request,
    ola::proto::STREAMING_NO_RESPONSE*,
    ola::rpc::RpcService::CompletionCallback*) {
  Universe *universe = m_universe_store->GetUniverse(request->universe());

  if (!universe) {
    return;
  }

  Client *client = GetClient(controller);
  DmxBuffer buffer;
  buffer.Set(request->data());

  uint8_t priority = ola::dmx::SOURCE_PRIORITY_DEFAULT;
  if (request->has_priority()) {
    priority = request->priority();
    priority = std::max(static_cast<uint8_t>(ola::dmx::SOURCE_PRIORITY_MIN),
                        priority);
    priority = std::min(static_cast<uint8_t>(ola::dmx::SOURCE_PRIORITY_MAX),
                        priority);
  }
  DmxSource source(buffer, *m_wake_up_time, priority);
  client->DMXReceived(request->universe(), source);
  universe->SourceClientDataChanged(client);
}

void OlaServerServiceImpl::SetUniverseName(
    RpcController* controller,
    const UniverseNameRequest* request,
    Ack*,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe) {
    return MissingUniverseError(controller);
  }

  universe->SetName(request->name());
}

void OlaServerServiceImpl::SetMergeMode(
    RpcController* controller,
    const MergeModeRequest* request,
    Ack*,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe) {
    return MissingUniverseError(controller);
  }

  Universe::merge_mode mode = request->merge_mode() == ola::proto::HTP ?
    Universe::MERGE_HTP : Universe::MERGE_LTP;
  universe->SetMergeMode(mode);
}

void OlaServerServiceImpl::PatchPort(
    RpcController* controller,
    const PatchPortRequest* request,
    Ack*,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  AbstractDevice *device =
    m_device_manager->GetDevice(request->device_alias());

  if (!device) {
    return MissingDeviceError(controller);
  }

  bool result;
  if (request->is_output()) {
    OutputPort *port = device->GetOutputPort(request->port_id());
    if (!port) {
      return MissingPortError(controller);
    }

    if (request->action() == ola::proto::PATCH) {
      result = m_port_manager->PatchPort(port, request->universe());
    } else {
      result = m_port_manager->UnPatchPort(port);
    }
  } else {
    InputPort *port = device->GetInputPort(request->port_id());
    if (!port) {
      return MissingPortError(controller);
    }

    if (request->action() == ola::proto::PATCH) {
      result = m_port_manager->PatchPort(port, request->universe());
    } else {
      result = m_port_manager->UnPatchPort(port);
    }
  }

  if (!result) {
    controller->SetFailed("Patch port request failed");
  }
}

void OlaServerServiceImpl::SetPortPriority(
    RpcController* controller,
    const ola::proto::PortPriorityRequest* request,
    Ack*,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  AbstractDevice *device =
      m_device_manager->GetDevice(request->device_alias());

  if (!device) {
    return MissingDeviceError(controller);
  }

  bool status;

  bool inherit_mode = true;
  uint8_t value = 0;
  if (request->priority_mode() == PRIORITY_MODE_STATIC) {
    if (request->has_priority()) {
      inherit_mode = false;
      value = request->priority();
    } else {
      OLA_INFO << "In Set Port Priority, override mode was set but the value "
                  "wasn't specified";
      controller->SetFailed(
          "Invalid SetPortPriority request, see logs for more info");
      return;
    }
  }

  if (request->is_output()) {
    OutputPort *port = device->GetOutputPort(request->port_id());
    if (!port) {
      return MissingPortError(controller);
    }

    if (inherit_mode) {
      status = m_port_manager->SetPriorityInherit(port);
    } else {
      status = m_port_manager->SetPriorityStatic(port, value);
    }
  } else {
    InputPort *port = device->GetInputPort(request->port_id());
    if (!port) {
      return MissingPortError(controller);
    }

    if (inherit_mode) {
      status = m_port_manager->SetPriorityInherit(port);
    } else {
      status = m_port_manager->SetPriorityStatic(port, value);
    }
  }

  if (!status) {
    controller->SetFailed(
        "Invalid SetPortPriority request, see logs for more info");
  }
}

void OlaServerServiceImpl::AddUniverse(
    const Universe * universe,
    ola::proto::UniverseInfoReply *universe_info_reply) const {
  UniverseInfo *universe_info = universe_info_reply->add_universe();
  universe_info->set_universe(universe->UniverseId());
  universe_info->set_name(universe->Name());
  universe_info->set_merge_mode(universe->MergeMode() == Universe::MERGE_HTP
      ? ola::proto::HTP : ola::proto::LTP);
  universe_info->set_input_port_count(universe->InputPortCount());
  universe_info->set_output_port_count(universe->OutputPortCount());
  universe_info->set_rdm_devices(universe->UIDCount());

  std::vector<InputPort*> input_ports;
  std::vector<InputPort*>::const_iterator input_it;
  universe->InputPorts(&input_ports);
  for (input_it = input_ports.begin();
       input_it != input_ports.end();
       input_it++) {
    PortInfo *pi = universe_info->add_input_ports();
    PopulatePort(**input_it, pi);
  }

  std::vector<OutputPort*> output_ports;
  std::vector<OutputPort*>::const_iterator output_it;
  universe->OutputPorts(&output_ports);
  for (output_it = output_ports.begin();
       output_it != output_ports.end();
       output_it++) {
    PortInfo *pi = universe_info->add_output_ports();
    PopulatePort(**output_it, pi);
  }
}

void OlaServerServiceImpl::GetUniverseInfo(
    RpcController* controller,
    const OptionalUniverseRequest* request,
    UniverseInfoReply* response,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);

  if (request->has_universe()) {
    // return info for a single universe
    Universe *universe = m_universe_store->GetUniverse(request->universe());
    if (!universe) {
      return MissingUniverseError(controller);
    }

    AddUniverse(universe, response);
  } else {
    // return all
    vector<Universe*> uni_list;
    m_universe_store->GetList(&uni_list);
    vector<Universe*>::const_iterator iter;

    for (iter = uni_list.begin(); iter != uni_list.end(); ++iter) {
      AddUniverse(*iter, response);
    }
  }
}

void OlaServerServiceImpl::GetPlugins(
    RpcController*,
    const PluginListRequest*,
    PluginListReply* response,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  vector<AbstractPlugin*> plugin_list;
  vector<AbstractPlugin*>::const_iterator iter;
  m_plugin_manager->Plugins(&plugin_list);

  for (iter = plugin_list.begin(); iter != plugin_list.end(); ++iter) {
    PluginInfo *plugin_info = response->add_plugin();
    AddPlugin(*iter, plugin_info);
  }
}

void OlaServerServiceImpl::ReloadPlugins(
    RpcController*,
    const ::ola::proto::PluginReloadRequest*,
    Ack*,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  if (m_reload_plugins_callback.get()) {
    m_reload_plugins_callback->Run();
  } else {
    OLA_WARN << "No plugin reload callback provided!";
  }
}

void OlaServerServiceImpl::GetPluginDescription(
    RpcController* controller,
    const ola::proto::PluginDescriptionRequest* request,
    ola::proto::PluginDescriptionReply* response,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  AbstractPlugin *plugin =
    m_plugin_manager->GetPlugin((ola_plugin_id) request->plugin_id());

  if (plugin) {
    response->set_name(plugin->Name());
    response->set_description(plugin->Description());
  } else {
    controller->SetFailed("Plugin not loaded");
  }
}

void OlaServerServiceImpl::GetPluginState(
    RpcController* controller,
    const ola::proto::PluginStateRequest* request,
    ola::proto::PluginStateReply* response,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  ola_plugin_id plugin_id = (ola_plugin_id) request->plugin_id();
  AbstractPlugin *plugin = m_plugin_manager->GetPlugin(plugin_id);

  if (plugin) {
    response->set_name(plugin->Name());
    response->set_enabled(plugin->IsEnabled());
    response->set_active(m_plugin_manager->IsActive(plugin_id));
    response->set_preferences_source(plugin->PreferenceConfigLocation());
    vector<AbstractPlugin*> conflict_list;
    m_plugin_manager->GetConflictList(plugin_id, &conflict_list);
    vector<AbstractPlugin*>::const_iterator iter = conflict_list.begin();
    for (; iter != conflict_list.end(); ++iter) {
      PluginInfo *plugin_info = response->add_conflicts_with();
      AddPlugin(*iter, plugin_info);
    }
  } else {
    controller->SetFailed("Plugin not loaded");
  }
}

void OlaServerServiceImpl::SetPluginState(
    RpcController *controller,
    const ola::proto::PluginStateChangeRequest* request,
    Ack*,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  ola_plugin_id plugin_id = (ola_plugin_id) request->plugin_id();
  AbstractPlugin *plugin = m_plugin_manager->GetPlugin(plugin_id);

  if (plugin) {
    OLA_DEBUG << "SetPluginState to " << request->enabled()
              << " for plugin " << plugin->Name();
    if (request->enabled()) {
      if (!m_plugin_manager->EnableAndStartPlugin(plugin_id)) {
        controller->SetFailed("Failed to start plugin: " + plugin->Name());
      }
    } else {
      m_plugin_manager->DisableAndStopPlugin(plugin_id);
    }
  }
}

void OlaServerServiceImpl::GetDeviceInfo(
    RpcController*,
    const DeviceInfoRequest* request,
    DeviceInfoReply* response,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  vector<device_alias_pair> device_list = m_device_manager->Devices();
  vector<device_alias_pair>::const_iterator iter;

  for (iter = device_list.begin(); iter != device_list.end(); ++iter) {
    if (request->has_plugin_id()) {
      if (iter->device->Owner()->Id() == request->plugin_id() ||
          request->plugin_id() == ola::OLA_PLUGIN_ALL) {
        AddDevice(iter->device, iter->alias, response);
      }
    } else {
      AddDevice(iter->device, iter->alias, response);
    }
  }
}

void OlaServerServiceImpl::GetCandidatePorts(
    RpcController* controller,
    const ola::proto::OptionalUniverseRequest* request,
    ola::proto::DeviceInfoReply* response,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  vector<device_alias_pair> device_list = m_device_manager->Devices();
  vector<device_alias_pair>::const_iterator iter;

  Universe *universe = NULL;

  if (request->has_universe()) {
    universe = m_universe_store->GetUniverse(request->universe());

    if (!universe) {
      return MissingUniverseError(controller);
    }
  }

  vector<InputPort*> input_ports;
  vector<OutputPort*> output_ports;
  vector<InputPort*>::const_iterator input_iter;
  vector<OutputPort*>::const_iterator output_iter;

  for (iter = device_list.begin(); iter != device_list.end(); ++iter) {
    AbstractDevice *device = iter->device;
    input_ports.clear();
    output_ports.clear();
    device->InputPorts(&input_ports);
    device->OutputPorts(&output_ports);

    bool seen_input_port = false;
    bool seen_output_port = false;
    unsigned int unpatched_input_ports = 0;
    unsigned int unpatched_output_ports = 0;

    if (universe) {
      for (input_iter = input_ports.begin(); input_iter != input_ports.end();
           input_iter++) {
        if ((*input_iter)->GetUniverse() == universe) {
          seen_input_port = true;
        } else if (!(*input_iter)->GetUniverse()) {
          unpatched_input_ports++;
        }
      }

      for (output_iter = output_ports.begin();
           output_iter != output_ports.end(); output_iter++) {
        if ((*output_iter)->GetUniverse() == universe) {
          seen_output_port = true;
        } else if (!(*output_iter)->GetUniverse()) {
          unpatched_output_ports++;
        }
      }
    } else {
      unpatched_input_ports = input_ports.size();
      unpatched_output_ports = output_ports.size();
    }

    bool can_bind_more_input_ports = (
      (!seen_output_port || device->AllowLooping()) &&
      (!seen_input_port || device->AllowMultiPortPatching()));

    bool can_bind_more_output_ports = (
      (!seen_input_port || device->AllowLooping()) &&
      (!seen_output_port || device->AllowMultiPortPatching()));

    if ((unpatched_input_ports == 0 || !can_bind_more_input_ports) &&
        (unpatched_output_ports == 0 || !can_bind_more_output_ports)) {
      continue;
    }

    // go ahead and create the device at this point
    DeviceInfo *device_info = response->add_device();
    device_info->set_device_alias(iter->alias);
    device_info->set_device_name(device->Name());
    device_info->set_device_id(device->UniqueId());

    if (device->Owner()) {
      device_info->set_plugin_id(device->Owner()->Id());
    }

    for (input_iter = input_ports.begin(); input_iter != input_ports.end();
         ++input_iter) {
      if ((*input_iter)->GetUniverse()) {
        continue;
      }
      if (!can_bind_more_input_ports) {
        break;
      }

      PortInfo *port_info = device_info->add_input_port();
      PopulatePort(**input_iter, port_info);

      if (!device->AllowMultiPortPatching()) {
        break;
      }
    }

    for (output_iter = output_ports.begin(); output_iter != output_ports.end();
        ++output_iter) {
      if ((*output_iter)->GetUniverse()) {
        continue;
      }
      if (!can_bind_more_output_ports) {
        break;
      }

      PortInfo *port_info = device_info->add_output_port();
      PopulatePort(**output_iter, port_info);

      if (!device->AllowMultiPortPatching()) {
        break;
      }
    }
  }
}

void OlaServerServiceImpl::ConfigureDevice(
    RpcController* controller,
    const DeviceConfigRequest* request,
    DeviceConfigReply* response,
    ola::rpc::RpcService::CompletionCallback* done) {
  AbstractDevice *device =
      m_device_manager->GetDevice(request->device_alias());
  if (!device) {
    MissingDeviceError(controller);
    done->Run();
    return;
  }

  device->Configure(controller, request->data(),
                    response->mutable_data(), done);
}

void OlaServerServiceImpl::GetUIDs(
    RpcController* controller,
    const ola::proto::UniverseRequest* request,
    ola::proto::UIDListReply* response,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe) {
    return MissingUniverseError(controller);
  }

  response->set_universe(universe->UniverseId());
  UIDSet uid_set;
  universe->GetUIDs(&uid_set);

  UIDSet::Iterator iter = uid_set.Begin();
  for (; iter != uid_set.End(); ++iter) {
    ola::proto::UID *uid = response->add_uid();
    SetProtoUID(*iter, uid);
  }
}

void OlaServerServiceImpl::ForceDiscovery(
    RpcController* controller,
    const ola::proto::DiscoveryRequest* request,
    ola::proto::UIDListReply *response,
    ola::rpc::RpcService::CompletionCallback* done) {
  Universe *universe = m_universe_store->GetUniverse(request->universe());

  if (universe) {
    unsigned int universe_id = request->universe();
    m_broker->RunRDMDiscovery(
        GetClient(controller),
        universe,
        request->full(),
        NewSingleCallback(this,
                          &OlaServerServiceImpl::RDMDiscoveryComplete,
                          universe_id,
                          done,
                          response));
  } else {
    ClosureRunner runner(done);
    MissingUniverseError(controller);
  }
}

void OlaServerServiceImpl::RDMCommand(
    RpcController* controller,
    const ola::proto::RDMRequest* request,
    ola::proto::RDMResponse* response,
    ola::rpc::RpcService::CompletionCallback* done) {
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe) {
    MissingUniverseError(controller);
    done->Run();
    return;
  }

  Client *client = GetClient(controller);
  UID source_uid = client->GetUID();

  UID destination(request->uid().esta_id(),
                  request->uid().device_id());

  RDMRequest::OverrideOptions options = RDMRequestOptionsFromProto(*request);

  ola::rdm::RDMRequest *rdm_request = NULL;
  if (request->is_set()) {
    rdm_request = new ola::rdm::RDMSetRequest(
        source_uid,
        destination,
        universe->GetRDMTransactionNumber(),
        1,  // port id
        request->sub_device(),
        request->param_id(),
        reinterpret_cast<const uint8_t*>(request->data().data()),
        request->data().size(),
        options);
  } else {
    rdm_request = new ola::rdm::RDMGetRequest(
        source_uid,
        destination,
        universe->GetRDMTransactionNumber(),
        1,  // port id
        request->sub_device(),
        request->param_id(),
        reinterpret_cast<const uint8_t*>(request->data().data()),
        request->data().size(),
        options);
  }

  ola::rdm::RDMCallback *callback =
    NewSingleCallback(
        this,
        &OlaServerServiceImpl::HandleRDMResponse,
        response,
        done,
        request->include_raw_response());

  m_broker->SendRDMRequest(client, universe, rdm_request, callback);
}

void OlaServerServiceImpl::RDMDiscoveryCommand(
    RpcController* controller,
    const ola::proto::RDMDiscoveryRequest* request,
    ola::proto::RDMResponse* response,
    ola::rpc::RpcService::CompletionCallback* done) {
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe) {
    MissingUniverseError(controller);
    done->Run();
    return;
  }

  Client *client = GetClient(controller);
  UID source_uid = client->GetUID();

  UID destination(request->uid().esta_id(),
                  request->uid().device_id());

  RDMRequest::OverrideOptions options = RDMRequestOptionsFromProto(*request);

  ola::rdm::RDMRequest *rdm_request = new ola::rdm::RDMDiscoveryRequest(
      source_uid,
      destination,
      universe->GetRDMTransactionNumber(),
      1,  // port id
      request->sub_device(),
      request->param_id(),
      reinterpret_cast<const uint8_t*>(request->data().data()),
      request->data().size(),
      options);

  ola::rdm::RDMCallback *callback =
    NewSingleCallback(
        this,
        &OlaServerServiceImpl::HandleRDMResponse,
        response,
        done,
        request->include_raw_response());

  m_broker->SendRDMRequest(client, universe, rdm_request, callback);
}

void OlaServerServiceImpl::SetSourceUID(
    RpcController *controller,
    const ola::proto::UID* request,
    Ack*,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);

  UID source_uid(request->esta_id(), request->device_id());
  GetClient(controller)->SetUID(source_uid);
}

void OlaServerServiceImpl::SendTimeCode(
    RpcController* controller,
    const ola::proto::TimeCode* request,
    Ack*,
    ola::rpc::RpcService::CompletionCallback* done) {
  ClosureRunner runner(done);
  ola::timecode::TimeCode time_code(
      static_cast<ola::timecode::TimeCodeType>(request->type()),
      request->hours(),
      request->minutes(),
      request->seconds(),
      request->frames());

  if (time_code.IsValid()) {
    m_device_manager->SendTimeCode(time_code);
  } else {
    controller->SetFailed("Invalid TimeCode");
  }
}


// Private methods
//-----------------------------------------------------------------------------
/*
 * Handle an RDM Response, this includes broadcast messages, messages that
 * timed out and normal response messages.
 */
void OlaServerServiceImpl::HandleRDMResponse(
    ola::proto::RDMResponse* response,
    ola::rpc::RpcService::CompletionCallback* done,
    bool include_raw_packets,
    ola::rdm::RDMReply *reply) {
  ClosureRunner runner(done);
  response->set_response_code(
      static_cast<ola::proto::RDMResponseCode>(reply->StatusCode()));

  if (reply->StatusCode() == ola::rdm::RDM_COMPLETED_OK) {
    if (!reply->Response()) {
      // No response returned.
      OLA_WARN << "RDM code was ok but response was NULL";
      response->set_response_code(static_cast<ola::proto::RDMResponseCode>(
            ola::rdm::RDM_INVALID_RESPONSE));
    } else if (reply->Response()->ResponseType() <= ola::rdm::RDM_NACK_REASON) {
      // Valid RDM Response code.
      SetProtoUID(reply->Response()->SourceUID(),
                  response->mutable_source_uid());
      SetProtoUID(reply->Response()->DestinationUID(),
                  response->mutable_dest_uid());
      response->set_transaction_number(reply->Response()->TransactionNumber());
      response->set_response_type(static_cast<ola::proto::RDMResponseType>(
          reply->Response()->ResponseType()));
      response->set_message_count(reply->Response()->MessageCount());
      response->set_sub_device(reply->Response()->SubDevice());

      switch (reply->Response()->CommandClass()) {
        case ola::rdm::RDMCommand::DISCOVER_COMMAND_RESPONSE:
          response->set_command_class(ola::proto::RDM_DISCOVERY_RESPONSE);
          break;
        case ola::rdm::RDMCommand::GET_COMMAND_RESPONSE:
          response->set_command_class(ola::proto::RDM_GET_RESPONSE);
          break;
        case ola::rdm::RDMCommand::SET_COMMAND_RESPONSE:
          response->set_command_class(ola::proto::RDM_SET_RESPONSE);
          break;
        default:
          OLA_WARN << "Unknown command class "
                   << strings::ToHex(static_cast<unsigned int>(
                         reply->Response()->CommandClass()));
      }

      response->set_param_id(reply->Response()->ParamId());

      if (reply->Response()->ParamData() &&
          reply->Response()->ParamDataSize()) {
        response->set_data(
            reinterpret_cast<const char*>(reply->Response()->ParamData()),
            reply->Response()->ParamDataSize());
      }
    } else {
      // Invalid RDM Response code.
      OLA_WARN << "RDM response present, but response type is invalid, was "
               << strings::ToHex(reply->Response()->ResponseType());
      response->set_response_code(ola::proto::RDM_INVALID_RESPONSE);
    }
  }

  if (include_raw_packets) {
    vector<rdm::RDMFrame>::const_iterator iter = reply->Frames().begin();
    for (; iter != reply->Frames().end(); ++iter) {
      ola::proto::RDMFrame *frame = response->add_raw_frame();
      frame->set_raw_response(iter->data.data(), iter->data.size());
      ola::proto::RDMFrameTiming *timing = frame->mutable_timing();
      timing->set_response_delay(iter->timing.response_time);
      timing->set_break_time(iter->timing.break_time);
      timing->set_mark_time(iter->timing.mark_time);
      timing->set_data_time(iter->timing.data_time);
    }
  }
}


/**
 * Called when RDM discovery completes
 */
void OlaServerServiceImpl::RDMDiscoveryComplete(
    unsigned int universe_id,
    ola::rpc::RpcService::CompletionCallback* done,
    ola::proto::UIDListReply *response,
    const UIDSet &uids) {
  ClosureRunner runner(done);

  response->set_universe(universe_id);
  UIDSet::Iterator iter = uids.Begin();
  for (; iter != uids.End(); ++iter) {
    ola::proto::UID *uid = response->add_uid();
    SetProtoUID(*iter, uid);
  }
}


void OlaServerServiceImpl::MissingUniverseError(RpcController* controller) {
  controller->SetFailed("Universe doesn't exist");
}

void OlaServerServiceImpl::MissingDeviceError(RpcController* controller) {
  controller->SetFailed("Device doesn't exist");
}


void OlaServerServiceImpl::MissingPluginError(RpcController* controller) {
  controller->SetFailed("Plugin doesn't exist");
}


void OlaServerServiceImpl::MissingPortError(RpcController* controller) {
  controller->SetFailed("Port doesn't exist");
}


/*
 * Add this device to the DeviceInfo response
 */
void OlaServerServiceImpl::AddPlugin(AbstractPlugin *plugin,
                                     PluginInfo* plugin_info) const {
  plugin_info->set_plugin_id(plugin->Id());
  plugin_info->set_name(plugin->Name());
  plugin_info->set_active(m_plugin_manager->IsActive(plugin->Id()));
  plugin_info->set_enabled(m_plugin_manager->IsEnabled(plugin->Id()));
}


/*
 * Add this device to the DeviceInfo response
 */
void OlaServerServiceImpl::AddDevice(AbstractDevice *device,
                                     unsigned int alias,
                                     DeviceInfoReply* response) const {
  DeviceInfo *device_info = response->add_device();
  device_info->set_device_alias(alias);
  device_info->set_device_name(device->Name());
  device_info->set_device_id(device->UniqueId());

  if (device->Owner()) {
    device_info->set_plugin_id(device->Owner()->Id());
  }

  vector<InputPort*> input_ports;
  device->InputPorts(&input_ports);
  vector<InputPort*>::const_iterator input_iter;

  for (input_iter = input_ports.begin(); input_iter != input_ports.end();
       ++input_iter) {
    PortInfo *port_info = device_info->add_input_port();
    PopulatePort(**input_iter, port_info);
  }

  vector<OutputPort*> output_ports;
  device->OutputPorts(&output_ports);
  vector<OutputPort*>::const_iterator output_iter;

  for (output_iter = output_ports.begin(); output_iter != output_ports.end();
      ++output_iter) {
    PortInfo *port_info = device_info->add_output_port();
    PopulatePort(**output_iter, port_info);
  }
}


template <class PortClass>
void OlaServerServiceImpl::PopulatePort(const PortClass &port,
                                        PortInfo *port_info) const {
  port_info->set_port_id(port.PortId());
  port_info->set_priority_capability(port.PriorityCapability());
  port_info->set_description(port.Description());

  if (port.GetUniverse()) {
    port_info->set_active(true);
    port_info->set_universe(port.GetUniverse()->UniverseId());
  } else {
    port_info->set_active(false);
  }

  if (port.PriorityCapability() != CAPABILITY_NONE) {
    port_info->set_priority_mode(port.GetPriorityMode());
    if (port.GetPriorityMode() == PRIORITY_MODE_STATIC) {
      port_info->set_priority(port.GetPriority());
    }
  }

  port_info->set_supports_rdm(port.SupportsRDM());
}

void OlaServerServiceImpl::SetProtoUID(const ola::rdm::UID &uid,
                                       ola::proto::UID *pb_uid) {
  pb_uid->set_esta_id(uid.ManufacturerId());
  pb_uid->set_device_id(uid.DeviceId());
}

Client* OlaServerServiceImpl::GetClient(ola::rpc::RpcController *controller) {
  return reinterpret_cast<Client*>(controller->Session()->GetData());
}
}  // namespace ola
