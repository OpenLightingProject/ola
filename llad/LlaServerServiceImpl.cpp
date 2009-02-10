
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
 e* LlaServerServiceImpl.cpp
 * Implemtation of the LlaServerService interface. This is the class that
 * handles all the RPCs on the server side.
 * Copyright (C) 2005 - 2008 Simon Newton
 */

#include <lla/ExportMap.h>
#include <llad/logger.h>
#include <llad/Universe.h>
#include <llad/Device.h>
#include <llad/Port.h>
#include <llad/Plugin.h>
#include <llad/PluginLoader.h>
#include "common/protocol/Lla.pb.h"
#include "LlaServerServiceImpl.h"
#include "UniverseStore.h"
#include "DeviceManager.h"

namespace lla {

using namespace lla::proto;


/*
 * Returns the current DMX values for a particular universe
 */
void LlaServerServiceImpl::GetDmx(
    RpcController* controller,
    const DmxReadRequest* request,
    DmxData* response,
    Closure* done) {

  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  int dmx_length;
  const uint8_t *dmx_data = universe->GetDMX(dmx_length);
  string dmx_string;
  dmx_string.append((char*) dmx_data, dmx_length);

  response->set_data(dmx_string);
  response->set_universe(request->universe());
  done->Run();
}


/*
 * Register a client to receive DMX data.
 */
void LlaServerServiceImpl::RegisterForDmx(
    RpcController* controller,
    const RegisterDmxRequest* request,
    Ack* response,
    Closure* done) {

  Universe *universe = m_universe_store->GetUniverseOrCreate(
                           request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  if (request->action() == REGISTER) {
    universe->AddClient(m_client);
  } else {
    universe->RemoveClient(m_client);
    m_universe_store->DeleteUniverseIfInactive(universe);
  }
  done->Run();
}


/*
 * Update the DMX values for a particular universe
 */
void LlaServerServiceImpl::UpdateDmxData(
    RpcController* controller,
    const DmxData* request,
    Ack* response,
    Closure* done) {

  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  string dmx_string = request->data();
  universe->SetDMX((uint8_t*) dmx_string.c_str(), dmx_string.length());
  done->Run();
}


/*
 * Sets the name of a universe
 */
void LlaServerServiceImpl::SetUniverseName(
    RpcController* controller,
    const UniverseNameRequest* request,
    Ack* response,
    Closure* done) {

  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  universe->SetName(request->name());
  done->Run();
}


/*
 * Set the merge mode for a universe
 */
void LlaServerServiceImpl::SetMergeMode(
    RpcController* controller,
    const MergeModeRequest* request,
    Ack* response,
    Closure* done) {

  Universe *universe = m_universe_store->GetUniverse(request->universe());
  if (!universe)
    return MissingUniverseError(controller, done);

  Universe::merge_mode mode = request->merge_mode() == HTP ? Universe::MERGE_HTP :
      Universe::MERGE_LTP;
  universe->SetMergeMode(mode);
  done->Run();
}


/*
 * Patch a port to a universe
 */
void LlaServerServiceImpl::PatchPort(
    RpcController* controller,
    const PatchPortRequest* request,
    Ack* response,
    Closure* done) {

  Universe *universe;
  AbstractDevice *device = m_device_manager->GetDevice(request->device_id());
  if (!device)
    return MissingDeviceError(controller, done);

  AbstractPort *port = device->GetPort(request->port_id());
  if (!port)
    return MissingPortError(controller, done);

  if (request->action() == PATCH) {
    universe = m_universe_store->GetUniverseOrCreate(request->universe());
    if (!universe)
      return MissingUniverseError(controller, done);

    universe->AddPort(port);
  } else {
    universe = port->GetUniverse();

    if (universe) {
      universe->RemovePort(port);
      m_universe_store->DeleteUniverseIfInactive(universe);
    }
  }
  done->Run();
}


/*
 * Returns information on the active universes.
 */
void LlaServerServiceImpl::GetUniverseInfo(
    RpcController* controller,
    const UniverseInfoRequest* request,
    UniverseInfoReply* response,
    Closure* done) {

  UniverseInfo *universe_info;

  if (request->has_universe()) {
    // return info for a single universe
    Universe *universe = m_universe_store->GetUniverse(request->universe());
    if (!universe)
      return MissingUniverseError(controller, done);

    universe_info = response->add_universe();
    universe_info->set_universe(universe->UniverseId());
    universe_info->set_name(universe->Name());
    universe_info->set_merge_mode(universe->MergeMode() == Universe::MERGE_HTP?
        HTP: LTP);
  } else {
    // return all
    vector<Universe *> *uni_list = m_universe_store->GetList();
    vector<Universe *>::const_iterator iter;

    for (iter = uni_list->begin(); iter != uni_list->end(); ++iter) {
      universe_info = response->add_universe();
      universe_info->set_universe((*iter)->UniverseId());
      universe_info->set_name((*iter)->Name());
      universe_info->set_merge_mode((*iter)->MergeMode() == Universe::MERGE_HTP?
        HTP: LTP);
    }
    delete uni_list;
  }
  done->Run();
}


/*
 * Return info on available plugins
 */
void LlaServerServiceImpl::GetPluginInfo(RpcController* controller,
                                         const PluginInfoRequest* request,
                                         PluginInfoReply* response,
                                         Closure* done) {
  vector<AbstractPlugin*> plugin_list = m_plugin_loader->Plugins();
  vector<AbstractPlugin*>::const_iterator iter;

  for (iter = plugin_list.begin(); iter != plugin_list.end(); ++iter) {
    if (! (request->has_plugin_id() && (*iter)->Id() != request->plugin_id()))
      AddPlugin(*iter, response, request->include_description());
  }
  if (!response->plugin_size() && request->has_plugin_id())
    controller->SetFailed("Plugin not loaded");
  done->Run();
}


/*
 * Return information on available devices
 */
void LlaServerServiceImpl::GetDeviceInfo(RpcController* controller,
                                         const DeviceInfoRequest* request,
                                         DeviceInfoReply* response,
                                         Closure* done) {

  vector<AbstractDevice *> device_list = m_device_manager->Devices();
  vector<AbstractDevice *>::const_iterator iter;

  for (iter = device_list.begin(); iter != device_list.end(); ++iter) {
    if (request->has_plugin_id()) {
      if ((*iter)->Owner()->Id() == request->plugin_id() ||
          request->plugin_id() == LLA_PLUGIN_ALL)
        AddDevice(*iter, response);
    } else {
      AddDevice(*iter, response);
    }
  }
  done->Run();
}


/*
 * Handle a ConfigureDevice request
 */
void LlaServerServiceImpl::ConfigureDevice(RpcController* controller,
                                           const DeviceConfigRequest* request,
                                           DeviceConfigReply* response,
                                           Closure* done) {

  AbstractDevice *device = m_device_manager->GetDevice(request->device_id());
  if (!device)
    return MissingDeviceError(controller, done);

  device->Configure(controller,
                    request->data(),
                    response->mutable_data(), done);
}


// Private method
//-------------------------------------------------------------------------------
void LlaServerServiceImpl::MissingUniverseError(RpcController* controller,
                                                Closure* done) {

  controller->SetFailed("Universe doesn't exist");
  done->Run();
}


void LlaServerServiceImpl::MissingDeviceError(RpcController* controller,
                                              Closure* done) {

  controller->SetFailed("Device doesn't exist");
  done->Run();
}


void LlaServerServiceImpl::MissingPluginError(RpcController* controller,
                                              Closure* done) {

  controller->SetFailed("Plugin doesn't exist");
  done->Run();
}


void LlaServerServiceImpl::MissingPortError(RpcController* controller,
                                            Closure* done) {

  controller->SetFailed("Port doesn't exist");
  done->Run();
}


/*
 * Add this device to the DeviceInfo response
 */
void LlaServerServiceImpl::AddPlugin(AbstractPlugin *plugin,
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
void LlaServerServiceImpl::AddDevice(AbstractDevice *device,
                                     DeviceInfoReply* response) const {

  DeviceInfo *device_info = response->add_device();
  device_info->set_device_id(device->DeviceId());
  device_info->set_device_name(device->Name());

  if (device->Owner())
    device_info->set_plugin_id(device->Owner()->Id());

  vector<AbstractPort*> ports = device->Ports();
  vector<AbstractPort*>::const_iterator iter;

    for (iter = ports.begin(); iter != ports.end(); ++iter) {
      PortInfo *port_info = device_info->add_port();
      port_info->set_port_id((*iter)->PortId());
      port_info->set_output_port((*iter)->CanWrite());

      if ((*iter)->GetUniverse()) {
        port_info->set_active(true);
        port_info->set_universe((*iter)->GetUniverse()->UniverseId());
      } else {
        port_info->set_active(false);
      }
    }
}


//LlaServerServiceImplFactory
//-------------------------------------------------------------------------------
LlaServerServiceImpl *LlaServerServiceImplFactory::New(
    UniverseStore *universe_store,
    DeviceManager *device_manager,
    PluginLoader *plugin_loader,
    Client *client,
    ExportMap *export_map) {
  return new LlaServerServiceImpl(universe_store,
                                  device_manager,
                                  plugin_loader,
                                  client,
                                  export_map);
};

} //lla
