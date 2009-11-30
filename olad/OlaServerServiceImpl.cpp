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

#include <string>
#include <vector>
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/ExportMap.h"
#include "olad/Universe.h"
#include "olad/Device.h"
#include "olad/Port.h"
#include "olad/Plugin.h"
#include "olad/PluginLoader.h"
#include "olad/PortPatcher.h"
#include "olad/Client.h"
#include "olad/DeviceManager.h"
#include "olad/OlaServerServiceImpl.h"
#include "olad/UniverseStore.h"
#include "common/protocol/Ola.pb.h"

namespace ola {

using ola::proto::Ack;
using ola::proto::DeviceConfigReply;
using ola::proto::DeviceConfigRequest;
using ola::proto::DeviceInfo;
using ola::proto::DeviceInfoReply;
using ola::proto::DeviceInfoRequest;
using ola::proto::DmxData;
using ola::proto::DmxReadRequest;
using ola::proto::MergeModeRequest;
using ola::proto::PatchPortRequest;
using ola::proto::PluginInfo;
using ola::proto::PluginInfoReply;
using ola::proto::PluginInfoRequest;
using ola::proto::PortInfo;
using ola::proto::RegisterDmxRequest;
using ola::proto::UniverseInfo;
using ola::proto::UniverseInfoReply;
using ola::proto::UniverseInfoRequest;
using ola::proto::UniverseNameRequest;
using google::protobuf::RpcController;


/*
 * Returns the current DMX values for a particular universe
 */
void OlaServerServiceImpl::GetDmx(
    RpcController* controller,
    const DmxReadRequest* request,
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

  DmxBuffer buffer;
  buffer.Set(request->data());
  if (m_client) {
    m_client->DMXRecieved(request->universe(), buffer);
    universe->SourceClientDataChanged(m_client);
  }
  done->Run();
  (void) response;
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
      result = m_port_patcher->PatchPort(port, request->universe());
    else
      result = m_port_patcher->UnPatchPort(port);
  } else {
    InputPort *port = device->GetInputPort(request->port_id());
    if (!port)
      return MissingPortError(controller, done);

    if (request->action() == ola::proto::PATCH)
      result = m_port_patcher->PatchPort(port, request->universe());
    else
      result = m_port_patcher->UnPatchPort(port);
  }

  if (!result)
    controller->SetFailed("Patch port request failed");
  (void) response;
  done->Run();
}


/*
 * Returns information on the active universes.
 */
void OlaServerServiceImpl::GetUniverseInfo(
    RpcController* controller,
    const UniverseInfoRequest* request,
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
    }
  }
  done->Run();
}


/*
 * Return info on available plugins
 */
void OlaServerServiceImpl::GetPluginInfo(RpcController* controller,
                                         const PluginInfoRequest* request,
                                         PluginInfoReply* response,
                                         google::protobuf::Closure* done) {
  vector<AbstractPlugin*> plugin_list = m_plugin_loader->Plugins();
  vector<AbstractPlugin*>::const_iterator iter;

  bool include_all = (!request->has_plugin_id() ||
      request->plugin_id() == ola::OLA_PLUGIN_ALL);

  for (iter = plugin_list.begin(); iter != plugin_list.end(); ++iter) {
    if (include_all || (*iter)->Id() == request->plugin_id())
      AddPlugin(*iter, response, request->include_description());
  }
  if (!response->plugin_size() && request->has_plugin_id())
    controller->SetFailed("Plugin not loaded");
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
                                     PluginInfoReply* response,
                                     bool include_description) const {
  PluginInfo *plugin_info = response->add_plugin();
  plugin_info->set_plugin_id(plugin->Id());
  plugin_info->set_name(plugin->Name());
  if (include_description) {
    plugin_info->set_description(plugin->Description());
  }
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
  port_info->set_description(port.Description());

  if (port.GetUniverse()) {
    port_info->set_active(true);
    port_info->set_universe(port.GetUniverse()->UniverseId());
  } else {
    port_info->set_active(false);
  }
}


// OlaServerServiceImplFactory
// -----------------------------------------------------------------------------
OlaServerServiceImpl *OlaServerServiceImplFactory::New(
    UniverseStore *universe_store,
    DeviceManager *device_manager,
    PluginLoader *plugin_loader,
    Client *client,
    ExportMap *export_map,
    PortPatcher *port_patcher) {
  return new OlaServerServiceImpl(universe_store,
                                  device_manager,
                                  plugin_loader,
                                  client,
                                  export_map,
                                  port_patcher);
};
}  // ola
