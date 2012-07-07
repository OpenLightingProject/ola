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
 * OlaServerServiceImpl.cpp
 * Implemtation of the OlaServerService interface. This is the class that
 * handles all the RPCs on the server side.
 * Copyright (C) 2005 - 2008 Simon Newton
 */

#include <algorithm>
#include <string>
#include <vector>
#include "common/protocol/Ola.pb.h"
#include "ola/Callback.h"
#include "ola/CallbackRunner.h"
#include "ola/DmxBuffer.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/rdm/UIDSet.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/timecode/TimeCode.h"
#include "ola/timecode/TimeCodeEnums.h"
#include "olad/Client.h"
#include "olad/Device.h"
#include "olad/DeviceManager.h"
#include "olad/DmxSource.h"
#include "olad/OlaServerServiceImpl.h"
#include "olad/Plugin.h"
#include "olad/PluginManager.h"
#include "olad/Port.h"
#include "olad/PortManager.h"
#include "olad/Universe.h"
#include "olad/UniverseStore.h"

namespace ola {

using google::protobuf::RpcController;
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
using ola::CallbackRunner;
using ola::rdm::UIDSet;
using ola::rdm::RDMResponse;


typedef CallbackRunner<google::protobuf::Closure> ClosureRunner;

OlaServerServiceImpl::~OlaServerServiceImpl() {
}


/*
 * Returns the current DMX values for a particular universe
 */
void OlaServerServiceImpl::GetDmx(
    RpcController* controller,
    const UniverseRequest* request,
    DmxData* response,
    google::protobuf::Closure* done) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller);

  const DmxBuffer buffer = universe->GetDMX();
  response->set_data(buffer.Get());
  response->set_universe(request->universe());
}



/*
 * Register a client to receive DMX data.
 */
void OlaServerServiceImpl::RegisterForDmx(
    RpcController* controller,
    const RegisterDmxRequest* request,
    Ack*,
    google::protobuf::Closure* done,
    Client *client) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverseOrCreate(
      request->universe());
  if (!universe)
    return MissingUniverseError(controller);

  if (request->action() == ola::proto::REGISTER) {
    universe->AddSinkClient(client);
  } else {
    universe->RemoveSinkClient(client);
  }
}


/*
 * Update the DMX values for a particular universe
 */
void OlaServerServiceImpl::UpdateDmxData(
    RpcController* controller,
    const DmxData* request,
    Ack*,
    google::protobuf::Closure* done,
    Client *client) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller);

  if (client) {
    DmxBuffer buffer;
    buffer.Set(request->data());

    uint8_t priority = DmxSource::PRIORITY_DEFAULT;
    if (request->has_priority()) {
      priority = request->priority();
      priority = std::max(DmxSource::PRIORITY_MIN, priority);
      priority = std::min(DmxSource::PRIORITY_MAX, priority);
    }
    DmxSource source(buffer, *m_wake_up_time, priority);
    client->DMXRecieved(request->universe(), source);
    universe->SourceClientDataChanged(client);
  }
}


/*
 * Handle a streaming DMX update, we don't send responses for this
 */
void OlaServerServiceImpl::StreamDmxData(
    RpcController*,
    const ::ola::proto::DmxData* request,
    ::ola::proto::STREAMING_NO_RESPONSE*,
    ::google::protobuf::Closure*,
    Client *client) {

  Universe *universe = m_universe_store->GetUniverse(request->universe());

  if (!universe)
    return;

  if (client) {
    DmxBuffer buffer;
    buffer.Set(request->data());

    uint8_t priority = DmxSource::PRIORITY_DEFAULT;
    if (request->has_priority()) {
      priority = request->priority();
      priority = std::max(DmxSource::PRIORITY_MIN, priority);
      priority = std::min(DmxSource::PRIORITY_MAX, priority);
    }
    DmxSource source(buffer, *m_wake_up_time, priority);
    client->DMXRecieved(request->universe(), source);
    universe->SourceClientDataChanged(client);
  }
}


/*
 * Sets the name of a universe
 */
void OlaServerServiceImpl::SetUniverseName(
    RpcController* controller,
    const UniverseNameRequest* request,
    Ack*,
    google::protobuf::Closure* done) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller);

  universe->SetName(request->name());
}


/*
 * Set the merge mode for a universe
 */
void OlaServerServiceImpl::SetMergeMode(
    RpcController* controller,
    const MergeModeRequest* request,
    Ack*,
    google::protobuf::Closure* done) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller);

  Universe::merge_mode mode = request->merge_mode() == ola::proto::HTP ?
    Universe::MERGE_HTP : Universe::MERGE_LTP;
  universe->SetMergeMode(mode);
}


/*
 * Patch a port to a universe
 */
void OlaServerServiceImpl::PatchPort(
    RpcController* controller,
    const PatchPortRequest* request,
    Ack*,
    google::protobuf::Closure* done) {
  ClosureRunner runner(done);
  AbstractDevice *device =
    m_device_manager->GetDevice(request->device_alias());

  if (!device)
    return MissingDeviceError(controller);

  bool result;
  if (request->is_output()) {
    OutputPort *port = device->GetOutputPort(request->port_id());
    if (!port)
      return MissingPortError(controller);

    if (request->action() == ola::proto::PATCH)
      result = m_port_manager->PatchPort(port, request->universe());
    else
      result = m_port_manager->UnPatchPort(port);
  } else {
    InputPort *port = device->GetInputPort(request->port_id());
    if (!port)
      return MissingPortError(controller);

    if (request->action() == ola::proto::PATCH)
      result = m_port_manager->PatchPort(port, request->universe());
    else
      result = m_port_manager->UnPatchPort(port);
  }

  if (!result)
    controller->SetFailed("Patch port request failed");
}


/*
 * Set the priority of a set of ports
 */
void OlaServerServiceImpl::SetPortPriority(
    RpcController* controller,
    const ola::proto::PortPriorityRequest* request,
    Ack*,
    google::protobuf::Closure* done) {
  ClosureRunner runner(done);
  AbstractDevice *device =
    m_device_manager->GetDevice(request->device_alias());

  if (!device)
    return MissingDeviceError(controller);

  bool status;

  bool inherit_mode = true;
  uint8_t value = 0;
  if (request->priority_mode() == PRIORITY_MODE_OVERRIDE) {
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
    if (!port)
      return MissingPortError(controller);

    if (inherit_mode)
      status = m_port_manager->SetPriorityInherit(port);
    else
      status = m_port_manager->SetPriorityOverride(port, value);
  } else {
    InputPort *port = device->GetInputPort(request->port_id());
    if (!port)
      return MissingPortError(controller);

    if (inherit_mode)
      status = m_port_manager->SetPriorityInherit(port);
    else
      status = m_port_manager->SetPriorityOverride(port, value);
  }

  if (!status)
    controller->SetFailed(
        "Invalid SetPortPriority request, see logs for more info");
}


/*
 * Returns information on the active universes.
 */
void OlaServerServiceImpl::GetUniverseInfo(
    RpcController* controller,
    const OptionalUniverseRequest* request,
    UniverseInfoReply* response,
    google::protobuf::Closure* done) {
  ClosureRunner runner(done);
  UniverseInfo *universe_info;

  if (request->has_universe()) {
    // return info for a single universe
    Universe *universe = m_universe_store->GetUniverse(request->universe());
    if (!universe)
      return MissingUniverseError(controller);

    universe_info = response->add_universe();
    universe_info->set_universe(universe->UniverseId());
    universe_info->set_name(universe->Name());
    universe_info->set_merge_mode(universe->MergeMode() == Universe::MERGE_HTP
        ? ola::proto::HTP: ola::proto::LTP);
    universe_info->set_input_port_count(universe->InputPortCount());
    universe_info->set_output_port_count(universe->OutputPortCount());
    universe_info->set_rdm_devices(universe->UIDCount());
  } else {
    // return all
    vector<Universe*> uni_list;
    m_universe_store->GetList(&uni_list);
    vector<Universe*>::const_iterator iter;

    for (iter = uni_list.begin(); iter != uni_list.end(); ++iter) {
      universe_info = response->add_universe();
      universe_info->set_universe((*iter)->UniverseId());
      universe_info->set_name((*iter)->Name());
      universe_info->set_merge_mode((*iter)->MergeMode() == Universe::MERGE_HTP
          ? ola::proto::HTP: ola::proto::LTP);
      universe_info->set_input_port_count((*iter)->InputPortCount());
      universe_info->set_output_port_count((*iter)->OutputPortCount());
      universe_info->set_rdm_devices((*iter)->UIDCount());
    }
  }
}


/*
 * Return info on available plugins
 */
void OlaServerServiceImpl::GetPlugins(
    RpcController*,
    const PluginListRequest*,
    PluginListReply* response,
    google::protobuf::Closure* done) {
  ClosureRunner runner(done);
  vector<AbstractPlugin*> plugin_list;
  vector<AbstractPlugin*>::const_iterator iter;
  m_plugin_manager->Plugins(&plugin_list);

  for (iter = plugin_list.begin(); iter != plugin_list.end(); ++iter)
    AddPlugin(*iter, response);
}


/*
 * Return the description for a plugin.
 */
void OlaServerServiceImpl::GetPluginDescription(
    RpcController* controller,
    const ola::proto::PluginDescriptionRequest* request,
    ola::proto::PluginDescriptionReply* response,
    google::protobuf::Closure* done) {
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


/*
 * Return information on available devices
 */
void OlaServerServiceImpl::GetDeviceInfo(RpcController*,
                                         const DeviceInfoRequest* request,
                                         DeviceInfoReply* response,
                                         google::protobuf::Closure* done) {
  ClosureRunner runner(done);
  vector<device_alias_pair> device_list = m_device_manager->Devices();
  vector<device_alias_pair>::const_iterator iter;

  for (iter = device_list.begin(); iter != device_list.end(); ++iter) {
    if (request->has_plugin_id()) {
      if (iter->device->Owner()->Id() == request->plugin_id() ||
          request->plugin_id() == ola::OLA_PLUGIN_ALL)
        AddDevice(iter->device, iter->alias, response);
    } else {
      AddDevice(iter->device, iter->alias, response);
    }
  }
}


/*
 * Handle a GetCandidatePorts request
 */
void OlaServerServiceImpl::GetCandidatePorts(
    RpcController* controller,
    const ola::proto::OptionalUniverseRequest* request,
    ola::proto::DeviceInfoReply* response,
    google::protobuf::Closure* done) {
  ClosureRunner runner(done);
  vector<device_alias_pair> device_list = m_device_manager->Devices();
  vector<device_alias_pair>::const_iterator iter;

  Universe *universe = NULL;

  if (request->has_universe()) {
    universe = m_universe_store->GetUniverse(request->universe());

    if (!universe)
      return MissingUniverseError(controller);
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
        if ((*input_iter)->GetUniverse() == universe)
          seen_input_port = true;
        else if (!(*input_iter)->GetUniverse())
          unpatched_input_ports++;
      }

      for (output_iter = output_ports.begin();
           output_iter != output_ports.end(); output_iter++) {
        if ((*output_iter)->GetUniverse() == universe)
          seen_output_port = true;
        else if (!(*output_iter)->GetUniverse())
          unpatched_output_ports++;
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
        (unpatched_output_ports == 0 || !can_bind_more_output_ports))
      continue;

    // go ahead and create the device at this point
    DeviceInfo *device_info = response->add_device();
    device_info->set_device_alias(iter->alias);
    device_info->set_device_name(device->Name());
    device_info->set_device_id(device->UniqueId());

    if (device->Owner())
      device_info->set_plugin_id(device->Owner()->Id());

    for (input_iter = input_ports.begin(); input_iter != input_ports.end();
         ++input_iter) {
      if ((*input_iter)->GetUniverse())
        continue;
      if (!can_bind_more_input_ports)
        break;

      PortInfo *port_info = device_info->add_input_port();
      PopulatePort(**input_iter, port_info);

      if (!device->AllowMultiPortPatching())
        break;
    }

    for (output_iter = output_ports.begin(); output_iter != output_ports.end();
        ++output_iter) {
      if ((*output_iter)->GetUniverse())
        continue;
      if (!can_bind_more_output_ports)
        break;

      PortInfo *port_info = device_info->add_output_port();
      PopulatePort(**output_iter, port_info);

      if (!device->AllowMultiPortPatching())
        break;
    }
  }
}


/*
 * Handle a ConfigureDevice request
 */
void OlaServerServiceImpl::ConfigureDevice(RpcController* controller,
                                           const DeviceConfigRequest* request,
                                           DeviceConfigReply* response,
                                           google::protobuf::Closure* done) {
  AbstractDevice *device =
    m_device_manager->GetDevice(request->device_alias());
  if (!device) {
    MissingDeviceError(controller);
    done->Run();
    return;
  }

  device->Configure(controller,
                    request->data(),
                    response->mutable_data(), done);
}


/*
 * Fetch the UID list for a universe
 */
void OlaServerServiceImpl::GetUIDs(RpcController* controller,
                                   const ola::proto::UniverseRequest* request,
                                   ola::proto::UIDListReply* response,
                                   google::protobuf::Closure* done) {
  ClosureRunner runner(done);
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller);

  response->set_universe(universe->UniverseId());
  UIDSet uid_set;
  universe->GetUIDs(&uid_set);

  UIDSet::Iterator iter = uid_set.Begin();
  for (; iter != uid_set.End(); ++iter) {
    ola::proto::UID *uid = response->add_uid();
    uid->set_esta_id(iter->ManufacturerId());
    uid->set_device_id(iter->DeviceId());
  }
}


/*
 * Force RDM discovery for a universe
 */
void OlaServerServiceImpl::ForceDiscovery(
    RpcController* controller,
    const ola::proto::DiscoveryRequest* request,
    ola::proto::UIDListReply *response,
    google::protobuf::Closure* done) {
  Universe *universe = m_universe_store->GetUniverse(request->universe());

  if (universe) {
    unsigned int universe_id = request->universe();
    universe->RunRDMDiscovery(
        NewSingleCallback(this,
                          &OlaServerServiceImpl::RDMDiscoveryComplete,
                          universe_id,
                          done,
                          response),
        request->full());
  } else {
    ClosureRunner runner(done);
    MissingUniverseError(controller);
  }
}


/*
 * Handle an RDM Command
 */
void OlaServerServiceImpl::RDMCommand(
    RpcController* controller,
    const ::ola::proto::RDMRequest* request,
    ola::proto::RDMResponse* response,
    google::protobuf::Closure* done,
    const UID *uid,
    class Client *client) {
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe) {
    MissingUniverseError(controller);
    done->Run();
    return;
  }

  UID source_uid = uid ? *uid : m_uid;
  UID destination(request->uid().esta_id(),
                  request->uid().device_id());

  ola::rdm::RDMRequest *rdm_request = NULL;
  if (request->is_set()) {
    rdm_request = new ola::rdm::RDMSetRequest(
      source_uid,
      destination,
      0,  // transaction #
      1,  // port id
      0,  // message count
      request->sub_device(),
      request->param_id(),
      reinterpret_cast<const uint8_t*>(request->data().data()),
      request->data().size());
  } else {
    rdm_request = new ola::rdm::RDMGetRequest(
      source_uid,
      destination,
      0,  // transaction #
      1,  // port id
      0,  // message count
      request->sub_device(),
      request->param_id(),
      reinterpret_cast<const uint8_t*>(request->data().data()),
      request->data().size());
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


/*
 * Handle an RDM Discovery Command. This should only be used for the RDM
 * responder tests.
 */
void OlaServerServiceImpl::RDMDiscoveryCommand(
    RpcController* controller,
    const ::ola::proto::RDMDiscoveryRequest* request,
    ola::proto::RDMResponse* response,
    google::protobuf::Closure* done,
    const UID *uid,
    class Client *client) {
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe) {
    MissingUniverseError(controller);
    done->Run();
    return;
  }

  UID source_uid = uid ? *uid : m_uid;
  UID destination(request->uid().esta_id(),
                  request->uid().device_id());

  ola::rdm::RDMRequest *rdm_request = new ola::rdm::RDMDiscoveryRequest(
      source_uid,
      destination,
      0,  // transaction #
      1,  // port id
      0,  // message count
      request->sub_device(),
      request->param_id(),
      reinterpret_cast<const uint8_t*>(request->data().data()),
      request->data().size());

  ola::rdm::RDMCallback *callback =
    NewSingleCallback(
        this,
        &OlaServerServiceImpl::HandleRDMResponse,
        response,
        done,
        request->include_raw_response());

  m_broker->SendRDMRequest(client, universe, rdm_request, callback);
}


/*
 * Set this client's source UID
 */
void OlaServerServiceImpl::SetSourceUID(
    RpcController*,
    const ::ola::proto::UID* request,
    ola::proto::Ack*,
    google::protobuf::Closure* done) {
  ClosureRunner runner(done);
  UID source_uid(request->esta_id(), request->device_id());
  m_uid = source_uid;
}


/**
 * Send Timecode
 */
void OlaServerServiceImpl::SendTimeCode(RpcController* controller,
                                        const ::ola::proto::TimeCode* request,
                                        ::ola::proto::Ack*,
                                        ::google::protobuf::Closure* done) {
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
    google::protobuf::Closure* done,
    bool include_raw_packets,
    ola::rdm::rdm_response_code code,
    const RDMResponse *rdm_response,
    const vector<string> &packets) {
  ClosureRunner runner(done);
  response->set_response_code(
      static_cast<ola::proto::RDMResponseCode>(code));

  if (code == ola::rdm::RDM_COMPLETED_OK) {
    if (!rdm_response) {
      OLA_WARN << "RDM code was ok but response was NULL";
      response->set_response_code(static_cast<ola::proto::RDMResponseCode>(
            ola::rdm::RDM_INVALID_RESPONSE));
    } else {
      uint8_t response_type = rdm_response->ResponseType();
      if (response_type <= ola::rdm::RDM_NACK_REASON) {
        response->set_response_type(
            static_cast<ola::proto::RDMResponseType>(response_type));
        response->set_message_count(rdm_response->MessageCount());
        response->set_param_id(rdm_response->ParamId());
        response->set_sub_device(rdm_response->SubDevice());

        switch (rdm_response->CommandClass()) {
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
            OLA_WARN << "Unknown command class 0x" << std::hex <<
              rdm_response->CommandClass();
        }

        if (rdm_response->ParamData() && rdm_response->ParamDataSize()) {
          const string data(
              reinterpret_cast<const char*>(rdm_response->ParamData()),
              rdm_response->ParamDataSize());
          response->set_data(data);
        } else {
          response->set_data("");
        }
      } else if (response) {
        OLA_WARN <<
          "RDM response present, but response type is invalid, was 0x" <<
          std::hex << static_cast<int>(response_type);
        response->set_response_code(static_cast<ola::proto::RDMResponseCode>(
              ola::rdm::RDM_INVALID_RESPONSE));
      }
    }
  }

  delete rdm_response;

  if (include_raw_packets) {
    vector<string>::const_iterator iter = packets.begin();
    for (; iter != packets.end(); ++iter) {
      response->add_raw_response(*iter);
    }
  }
}


/**
 * Called when RDM discovery completes
 */
void OlaServerServiceImpl::RDMDiscoveryComplete(
    unsigned int universe_id,
    google::protobuf::Closure* done,
    ola::proto::UIDListReply *response,
    const UIDSet &uids) {
  ClosureRunner runner(done);

  response->set_universe(universe_id);
  UIDSet::Iterator iter = uids.Begin();
  for (; iter != uids.End(); ++iter) {
    ola::proto::UID *uid = response->add_uid();
    uid->set_esta_id(iter->ManufacturerId());
    uid->set_device_id(iter->DeviceId());
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
                                     PluginListReply* response) const {
  PluginInfo *plugin_info = response->add_plugin();
  plugin_info->set_plugin_id(plugin->Id());
  plugin_info->set_name(plugin->Name());
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

  if (device->Owner())
    device_info->set_plugin_id(device->Owner()->Id());

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

  if (port.PriorityCapability() != CAPABILITY_NONE)
    port_info->set_priority(port.GetPriority());
  if (port.PriorityCapability() == CAPABILITY_FULL)
    port_info->set_priority_mode(port.GetPriorityMode());

  port_info->set_supports_rdm(port.SupportsRDM());
}


// OlaClientService
// ----------------------------------------------------------------------------
OlaClientService::~OlaClientService() {
  if (m_uid)
    delete m_uid;
}


/*
 * Set this client's source UID
 */
void OlaClientService::SetSourceUID(
    RpcController* controller,
    const ::ola::proto::UID* request,
    ola::proto::Ack* response,
    google::protobuf::Closure* done) {

  UID source_uid(request->esta_id(), request->device_id());
  if (!m_uid)
    m_uid = new UID(source_uid);
  else
    *m_uid = source_uid;
  done->Run();
  (void) controller;
  (void) response;
}


// OlaServerServiceImplFactory
// ----------------------------------------------------------------------------
OlaClientService *OlaClientServiceFactory::New(
    Client *client,
    OlaServerServiceImpl *impl) {
  return new OlaClientService(client, impl);
};
}  // ola
