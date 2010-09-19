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
#include "ola/DmxBuffer.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/rdm/UIDSet.h"
#include "olad/Client.h"
#include "olad/Device.h"
#include "olad/DeviceManager.h"
#include "olad/DmxSource.h"
#include "olad/InternalRDMController.h"
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
using ola::rdm::UIDSet;


OlaServerServiceImpl::~OlaServerServiceImpl() {
  if (m_uid)
    delete m_uid;
}


/*
 * Returns the current DMX values for a particular universe
 */
void OlaServerServiceImpl::GetDmx(
    RpcController* controller,
    const UniverseRequest* request,
    DmxData* response,
    google::protobuf::Closure* done) {

  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  const DmxBuffer buffer = universe->GetDMX();
  response->set_data(buffer.Get());
  response->set_universe(request->universe());
  done->Run();
}



/*
 * Register a client to receive DMX data.
 */
void OlaServerServiceImpl::RegisterForDmx(
    RpcController* controller,
    const RegisterDmxRequest* request,
    Ack* response,
    google::protobuf::Closure* done) {

  Universe *universe = m_universe_store->GetUniverseOrCreate(
      request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  if (request->action() == ola::proto::REGISTER) {
    universe->AddSinkClient(m_client);
  } else {
    universe->RemoveSinkClient(m_client);
  }
  done->Run();

  (void) response;
}


/*
 * Update the DMX values for a particular universe
 */
void OlaServerServiceImpl::UpdateDmxData(
    RpcController* controller,
    const DmxData* request,
    Ack* response,
    google::protobuf::Closure* done) {

  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  if (m_client) {
    DmxBuffer buffer;
    buffer.Set(request->data());

    uint8_t priority = DmxSource::PRIORITY_DEFAULT;
    if (request->has_priority()) {
      priority = request->priority();
      priority = std::max(DmxSource::PRIORITY_MIN, priority);
      priority = std::min(DmxSource::PRIORITY_MAX, priority);
    }
    DmxSource source(buffer, *m_wake_up_time, priority);
    m_client->DMXRecieved(request->universe(), source);
    universe->SourceClientDataChanged(m_client);
  }
  done->Run();
  (void) response;
}


/*
 * Handle a streaming DMX update, we don't send responses for this
 */
void OlaServerServiceImpl::StreamDmxData(
    RpcController* controller,
    const ::ola::proto::DmxData* request,
    ::ola::proto::STREAMING_NO_RESPONSE* response,
    ::google::protobuf::Closure* done) {

  Universe *universe = m_universe_store->GetUniverse(request->universe());

  if (!universe)
    return;

  if (m_client) {
    DmxBuffer buffer;
    buffer.Set(request->data());

    uint8_t priority = DmxSource::PRIORITY_DEFAULT;
    if (request->has_priority()) {
      priority = request->priority();
      priority = std::max(DmxSource::PRIORITY_MIN, priority);
      priority = std::min(DmxSource::PRIORITY_MAX, priority);
    }
    DmxSource source(buffer, *m_wake_up_time, priority);
    m_client->DMXRecieved(request->universe(), source);
    universe->SourceClientDataChanged(m_client);
  }
  (void) controller;
  (void) response;
  (void) done;
}


/*
 * Sets the name of a universe
 */
void OlaServerServiceImpl::SetUniverseName(
    RpcController* controller,
    const UniverseNameRequest* request,
    Ack* response,
    google::protobuf::Closure* done) {

  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  universe->SetName(request->name());
  done->Run();
  (void) response;
}


/*
 * Set the merge mode for a universe
 */
void OlaServerServiceImpl::SetMergeMode(
    RpcController* controller,
    const MergeModeRequest* request,
    Ack* response,
    google::protobuf::Closure* done) {

  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  Universe::merge_mode mode = request->merge_mode() == ola::proto::HTP ?
    Universe::MERGE_HTP : Universe::MERGE_LTP;
  universe->SetMergeMode(mode);
  done->Run();
  (void) response;
}


/*
 * Patch a port to a universe
 */
void OlaServerServiceImpl::PatchPort(
    RpcController* controller,
    const PatchPortRequest* request,
    Ack* response,
    google::protobuf::Closure* done) {

  AbstractDevice *device =
    m_device_manager->GetDevice(request->device_alias());

  if (!device)
    return MissingDeviceError(controller, done);

  bool result;
  if (request->is_output()) {
    OutputPort *port = device->GetOutputPort(request->port_id());
    if (!port)
      return MissingPortError(controller, done);

    if (request->action() == ola::proto::PATCH)
      result = m_port_manager->PatchPort(port, request->universe());
    else
      result = m_port_manager->UnPatchPort(port);
  } else {
    InputPort *port = device->GetInputPort(request->port_id());
    if (!port)
      return MissingPortError(controller, done);

    if (request->action() == ola::proto::PATCH)
      result = m_port_manager->PatchPort(port, request->universe());
    else
      result = m_port_manager->UnPatchPort(port);
  }

  if (!result)
    controller->SetFailed("Patch port request failed");
  (void) response;
  done->Run();
}


/*
 * Set the priority of a set of ports
 */
void OlaServerServiceImpl::SetPortPriority(
    RpcController* controller,
    const ola::proto::PortPriorityRequest* request,
    Ack* response,
    google::protobuf::Closure* done) {

  AbstractDevice *device =
    m_device_manager->GetDevice(request->device_alias());

  if (!device)
    return MissingDeviceError(controller, done);

  bool status;

  if (request->is_output()) {
    OutputPort *port = device->GetOutputPort(request->port_id());
    if (!port)
      return MissingPortError(controller, done);

    status = m_port_manager->SetPriority(port,
                                         request->priority_mode(),
                                         request->priority());
  } else {
    InputPort *port = device->GetInputPort(request->port_id());
    if (!port)
      return MissingPortError(controller, done);

    status = m_port_manager->SetPriority(port,
                                         request->priority_mode(),
                                         request->priority());
  }

  if (!status)
    controller->SetFailed(
        "Invalid SetPortPriority request, see logs for more info");
  done->Run();
  (void) response;
}


/*
 * Returns information on the active universes.
 */
void OlaServerServiceImpl::GetUniverseInfo(
    RpcController* controller,
    const OptionalUniverseRequest* request,
    UniverseInfoReply* response,
    google::protobuf::Closure* done) {

  UniverseInfo *universe_info;

  if (request->has_universe()) {
    // return info for a single universe
    Universe *universe = m_universe_store->GetUniverse(request->universe());
    if (!universe)
      return MissingUniverseError(controller, done);

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
  done->Run();
}


/*
 * Return info on available plugins
 */
void OlaServerServiceImpl::GetPlugins(
    RpcController* controller,
    const PluginListRequest* request,
    PluginListReply* response,
    google::protobuf::Closure* done) {
  vector<AbstractPlugin*> plugin_list;
  vector<AbstractPlugin*>::const_iterator iter;
  m_plugin_manager->Plugins(&plugin_list);

  for (iter = plugin_list.begin(); iter != plugin_list.end(); ++iter)
    AddPlugin(*iter, response);
  done->Run();
  (void) request;
  (void) controller;
}


/*
 * Return the description for a plugin.
 */
void OlaServerServiceImpl::GetPluginDescription(
    RpcController* controller,
    const ola::proto::PluginDescriptionRequest* request,
    ola::proto::PluginDescriptionReply* response,
    google::protobuf::Closure* done) {

  AbstractPlugin *plugin =
    m_plugin_manager->GetPlugin((ola_plugin_id) request->plugin_id());

  if (plugin) {
    response->set_name(plugin->Name());
    response->set_description(plugin->Description());
  } else {
    controller->SetFailed("Plugin not loaded");
  }
  done->Run();
}


/*
 * Return information on available devices
 */
void OlaServerServiceImpl::GetDeviceInfo(RpcController* controller,
                                         const DeviceInfoRequest* request,
                                         DeviceInfoReply* response,
                                         google::protobuf::Closure* done) {
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
  (void) controller;
  done->Run();
}


/*
 * Handle a GetCandidatePorts request
 */
void OlaServerServiceImpl::GetCandidatePorts(
    RpcController* controller,
    const ola::proto::OptionalUniverseRequest* request,
    ola::proto::DeviceInfoReply* response,
    google::protobuf::Closure* done) {
  vector<device_alias_pair> device_list = m_device_manager->Devices();
  vector<device_alias_pair>::const_iterator iter;

  Universe *universe = NULL;

  if (request->has_universe()) {
    universe = m_universe_store->GetUniverse(request->universe());

    if (!universe)
      return MissingUniverseError(controller, done);
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
  done->Run();
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
  if (!device)
    return MissingDeviceError(controller, done);

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
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  response->set_universe(request->universe());
  UIDSet uid_set;
  universe->GetUIDs(&uid_set);

  UIDSet::Iterator iter = uid_set.Begin();
  for (; iter != uid_set.End(); ++iter) {
    ola::proto::UID *uid = response->add_uid();
    uid->set_esta_id(iter->ManufacturerId());
    uid->set_device_id(iter->DeviceId());
  }
  done->Run();
}


/*
 * Force RDM discovery for a universe
 */
void OlaServerServiceImpl::ForceDiscovery(
    RpcController* controller,
    const ola::proto::UniverseRequest* request,
    ola::proto::Ack* response,
    google::protobuf::Closure* done) {

  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  universe->RunRDMDiscovery();
  done->Run();
  (void) response;
}


/*
 * Handle an RDM Command
 */
void OlaServerServiceImpl::RDMCommand(
    RpcController* controller,
    const ::ola::proto::RDMRequest* request,
    ola::proto::RDMResponse* response,
    google::protobuf::Closure* done) {
  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  UID destination(request->uid().esta_id(),
                  request->uid().device_id());

  SingleUseCallback1<void, const rdm_response_data&> *callback =
    NewSingleCallback(
        this,
        &OlaServerServiceImpl::HandleRDMResponse,
        controller,
        response,
        done);
  bool r = m_rdm_controller->SendRDMRequest(
    universe,
    destination,
    request->sub_device(),
    request->param_id(),
    request->data(),
    request->is_set(),
    callback,
    m_uid);
  if (!r) {
    controller->SetFailed("Failed to send RDM command");
    delete callback;
    done->Run();
  }
}


/*
 * Set this client's source UID
 */
void OlaServerServiceImpl::SetSourceUID(
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


/*
 * Handle an RDM Response, this includes broadcast messages, messages that
 * timed out and normal response messages.
 */
void OlaServerServiceImpl::HandleRDMResponse(
    RpcController* controller,
    ola::proto::RDMResponse* response,
    google::protobuf::Closure* done,
    const rdm_response_data &status) {

  // check for time out errors
  if (status.status == RDM_RESPONSE_TIMED_OUT) {
    controller->SetFailed("RDM command timed out");
    done->Run();
    return;
  }

  if (status.status == RDM_RESPONSE_OK) {
    if (status.response) {
      response->set_response_code(status.response->ResponseType());
      response->set_message_count(status.response->MessageCount());
      response->set_was_broadcast(false);

      if (status.response->ParamData() && status.response->ParamDataSize()) {
        const string data(
            reinterpret_cast<const char*>(status.response->ParamData()),
            status.response->ParamDataSize());
        response->set_data(data);
      } else {
        response->set_data("");
      }
    } else {
      OLA_WARN << "RDM state was ok but response was NULL";
      controller->SetFailed("Missing Response");
    }
  } else if (status.status == RDM_RESPONSE_BROADCAST) {
    response->set_was_broadcast(true);
    // fill these in with dummy values
    response->set_response_code(0);
    response->set_message_count(0);
    response->set_data("");
  } else {
    OLA_WARN << "unknown response status " << status.status;
    controller->SetFailed("Unknown status, see the logs");
  }

  done->Run();
}


// Private methods
//-----------------------------------------------------------------------------
void OlaServerServiceImpl::MissingUniverseError(
    RpcController* controller,
    google::protobuf::Closure* done) {
  controller->SetFailed("Universe doesn't exist");
  done->Run();
}

void OlaServerServiceImpl::MissingDeviceError(
    RpcController* controller,
    google::protobuf::Closure* done) {
  controller->SetFailed("Device doesn't exist");
  done->Run();
}


void OlaServerServiceImpl::MissingPluginError(
    RpcController* controller,
    google::protobuf::Closure* done) {
  controller->SetFailed("Plugin doesn't exist");
  done->Run();
}


void OlaServerServiceImpl::MissingPortError(RpcController* controller,
                                            google::protobuf::Closure* done) {
  controller->SetFailed("Port doesn't exist");
  done->Run();
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
}


// OlaServerServiceImplFactory
// ----------------------------------------------------------------------------
OlaServerServiceImpl *OlaServerServiceImplFactory::New(
    UniverseStore *universe_store,
    DeviceManager *device_manager,
    PluginManager *plugin_manager,
    Client *client,
    ExportMap *export_map,
    PortManager *port_manager,
    InternalRDMController *rdm_controller,
    const TimeStamp *wake_up_time) {
  return new OlaServerServiceImpl(universe_store,
                                  device_manager,
                                  plugin_manager,
                                  client,
                                  export_map,
                                  port_manager,
                                  rdm_controller,
                                  wake_up_time);
};
}  // ola
