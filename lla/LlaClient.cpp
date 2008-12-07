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
 * LlaClient.cpp
 * Implementation of LlaClient
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

#include <google/protobuf/stubs/common.h>
#include <lla/select_server/Socket.h>

#include "common/protocol/Lla.pb.h"
#include "common/rpc/StreamRpcChannel.h"
#include "common/rpc/SimpleRpcController.h"

#include "LlaClient.h"
#include "LlaClientServiceImpl.h"
#include "LlaDevice.h"


#define min(a,b)    ((a) < (b) ? (a) : (b))
#define max(a,b)    ((a) > (b) ? (a) : (b))

#ifdef HAVE_PTHREAD
#define acquire_lock pthread_mutex_lock(&m_mutex)
#define release_lock pthread_mutex_unlock(&m_mutex)
#else
#define acquire_lock
#define release_lock
#endif

namespace lla {

using google::protobuf::NewCallback;
using google::protobuf::Closure;
using lla::select_server::ConnectedSocket;
using lla::rpc::StreamRpcChannel;
using lla::rpc::SimpleRpcController;

LlaClient::LlaClient(ConnectedSocket *socket):
  m_socket(socket),
  m_client_service(NULL),
  m_channel(NULL),
  m_stub(NULL),
  m_connected(false),
  m_observer(NULL) {
#ifdef HAVE_PTHREAD
  pthread_mutex_init(&m_mutex, NULL);
#endif
}

LlaClient::~LlaClient() {
  if(m_connected)
    Stop();
}


/*
 * Setup this client
 */
bool LlaClient::Setup() {
  m_client_service = new LlaClientServiceImpl(m_observer);

  if (!m_client_service)
    return false;

  m_channel = new StreamRpcChannel(m_client_service, m_socket);

  if (!m_channel) {
    delete m_client_service;
    return false;
  }
  m_stub = new LlaServerService_Stub(m_channel);

  if (!m_stub) {
    delete m_channel;
    delete m_client_service;
    return false;
  }

  return true;
}


/*
 * Close the lla connection. This call may block.
 *
 * @return 0 on sucess, -1 on failure
 */
bool LlaClient::Stop() {
  acquire_lock;
  if(m_connected) {
    /*
    send_fin();

    while(m_connected) {
      switch(receive(1)) {
        case 0:
          break;
        case 1:
          // timeout
          printf("failed to get fin ack\n");
          goto e_error;
        case -1:
          // error
          goto e_error;
        case -2:
          // interupt
          break;
      }
    }
    close(m_sd);
    clear_plugins();
    clear_devices();
    clear_universes();
    m_connected = false;
    */
  }
  release_lock;
  return 0;
/*
e_error:
  release_lock;
  return -1;
  */
}


/*
 * set the observer object
 */
int LlaClient::SetObserver(LlaClientObserver *observer) {
  m_observer = observer;
  return 0;
}



bool LlaClient::FetchPluginInfo(int plugin_id, bool include_description) {
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::PluginInfoRequest request;
  lla::proto::PluginInfoReply *reply = new lla::proto::PluginInfoReply();

  if (plugin_id > 0)
    request.set_plugin_id(plugin_id);
  request.set_include_description(include_description);

  Closure *cb = NewCallback(this,
                            &lla::LlaClient::HandlePluginInfo,
                            controller,
                            reply);
  m_stub->GetPluginInfo(controller, &request, reply, cb);
  return true;
}





/*
 * Write some dmx data
 *
 * @param   c     lla_con
 * @param   uni   universe to send to
 * @param  data  dmx data
 * @param   length  length of dmx data
 * @return  0 on sucess, -1 on failure
 */
bool LlaClient::SendDmx(unsigned int universe, uint8_t *data, unsigned int length) {
  unsigned int dmx_length = min(length, MAX_DMX);
  lla::proto::DmxData request;
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::Ack *reply = new lla::proto::Ack();

  string dmx_data;
  dmx_data.append((char*) data, dmx_length);
  request.set_universe(universe);
  request.set_data(dmx_data);

  Closure *cb = NewCallback(this,
                            &lla::LlaClient::HandleSendDmx,
                            controller,
                            reply);
  m_stub->UpdateDmxData(controller, &request, reply, cb);
  return true;
}


/*
 * Read dmx data
 *
 * @param universe the universe id to get data for
 */
int LlaClient::FetchDmx(unsigned int universe) {
  lla::proto::DmxReadRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::DmxData *reply = new lla::proto::DmxData();

  request.set_universe(universe);

  Closure *cb = NewCallback(this,
                            &lla::LlaClient::HandleGetDmx,
                            controller,
                            reply);
  m_stub->GetDmx(controller, &request, reply, cb);
  return true;
}


/*
 * Request a listing of what devices are attached
 *
 * @param filter only get
 */
int LlaClient::FetchDeviceInfo(lla_plugin_id filter) {
  lla::proto::DeviceInfoRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::DeviceInfoReply *reply = new lla::proto::DeviceInfoReply();

  if (filter != LLA_PLUGIN_ALL)
    request.set_plugin_id(filter);

  Closure *cb = NewCallback(this,
                            &lla::LlaClient::HandleDeviceInfo,
                            controller,
                            reply);
  m_stub->GetDeviceInfo(controller, &request, reply, cb);
  return true;
}


/*
 * Request a universe listing
 *
 * @return
 */
int LlaClient::FetchUniverseInfo() {
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::UniverseInfoRequest request;
  lla::proto::UniverseInfoReply *reply = new lla::proto::UniverseInfoReply();

  Closure *cb = NewCallback(this,
                            &lla::LlaClient::HandleUniverseInfo,
                            controller,
                            reply);
  m_stub->GetUniverseInfo(controller, &request, reply, cb);
  return true;
}


/*
 * Set the name of a universe
 *
 */
int LlaClient::SetUniverseName(unsigned int universe, const string &name) {
  lla::proto::UniverseNameRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::Ack *reply = new lla::proto::Ack();

  request.set_universe(universe);
  request.set_name(name);

  Closure *cb = NewCallback(this,
                            &lla::LlaClient::HandleUniverseName,
                            controller,
                            reply);
  m_stub->SetUniverseName(controller, &request, reply, cb);
  return true;
}


/*
 * Set the merge mode of a universe
 *
 */
int LlaClient::SetUniverseMergeMode(unsigned int universe,
                                    LlaUniverse::merge_mode mode) {
  lla::proto::MergeModeRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::Ack *reply = new lla::proto::Ack();

  lla::proto::MergeMode merge_mode = mode == LlaUniverse::MERGE_HTP ? HTP : LTP;
  request.set_universe(universe);
  request.set_merge_mode(merge_mode);

  Closure *cb = NewCallback(this,
                            &lla::LlaClient::HandleUniverseMergeMode,
                            controller,
                            reply);
  m_stub->SetMergeMode(controller, &request, reply, cb);
  return true;
}


/*
 * Register our interest in a universe, the observer object will then
 * be notifed when the dmx values in this universe change.
 *
 * @param uni  the universe id
 * @param action
 */
int LlaClient::RegisterUniverse(unsigned int universe,
                                LlaClient::RegisterAction register_action) {
  lla::proto::RegisterDmxRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::Ack *reply = new lla::proto::Ack();

  lla::proto::RegisterAction action = (
      register_action == REGISTER ? lla::proto::REGISTER : lla::proto::UNREGISTER);
  request.set_universe(universe);
  request.set_action(action);

  Closure *cb = NewCallback(this,
                            &lla::LlaClient::HandleRegister,
                            controller,
                            reply);
  m_stub->RegisterForDmx(controller, &request, reply, cb);
  return true;
}


/*
 * (Un)Patch a port to a universe
 *
 * @param dev     the device id
 * @param port    the port id
 * @param action  LlaClient::PATCH or LlaClient::UNPATCH
 * @param uni    universe id
 */
int LlaClient::Patch(unsigned int device_id,
                     unsigned int port_id,
                     LlaClient::PatchAction patch_action,
                     unsigned int universe) {
  lla::proto::PatchPortRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::Ack *reply = new lla::proto::Ack();

  lla::proto::PatchAction action = (
      patch_action == PATCH ? lla::proto::PATCH : lla::proto::UNPATCH);
  request.set_universe(universe);
  request.set_device_id(device_id);
  request.set_port_id(port_id);
  request.set_action(action);

  Closure *cb = NewCallback(this,
                            &lla::LlaClient::HandlePatch,
                            controller,
                            reply);
  m_stub->PatchPort(controller, &request, reply, cb);
  return true;
}


/*
 * Sends a device config request
 *
 * @param dev   the device id
 * @param req  the request buffer
 * @param len  the length of the request buffer
 */
int LlaClient::ConfigureDevice(unsigned int device_id, const string &msg) {

  lla::proto::DeviceConfigRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::DeviceConfigReply *reply = new lla::proto::DeviceConfigReply();

  string configure_request;
  //configure_request.append(
  request.set_device_id(device_id);
  request.set_data(msg);

  Closure *cb = NewCallback(this,
                            &lla::LlaClient::HandleDeviceConfig,
                            controller,
                            reply);
  m_stub->ConfigureDevice(controller, &request, reply, cb);
  return true;
}


// The following are RPC callbacks

/*
 * Called once PluginInfo completes
 */
void LlaClient::HandlePluginInfo(SimpleRpcController *controller,
                                 lla::proto::PluginInfoReply *reply) {
  string error_string = "";
  vector<LlaPlugin> lla_plugins;

  if (m_observer) {
    if (controller->Failed())
      error_string = controller->ErrorText();
    else {
      for (int i=0; i < reply->plugin_size(); ++i) {
        PluginInfo plugin_info = reply->plugin(i);
        LlaPlugin plugin(plugin_info.plugin_id(), plugin_info.name());
        if (plugin_info.has_description())
          plugin.SetDescription(plugin_info.description());
        lla_plugins.push_back(plugin);
      }
    }
    release_lock;
    m_observer->Plugins(lla_plugins, error_string);
    acquire_lock;
  }
  delete controller;
  delete reply;
}


/*
 * Called once UpdateDmxData completes
 */
void LlaClient::HandleSendDmx(SimpleRpcController *controller,
                              lla::proto::Ack *reply) {
  if (controller->Failed())
    printf("send dmx failed: %s\n", controller->ErrorText().c_str());

  delete controller;
  delete reply;
}


/*
 * Called once GetDmx completes
 */
void LlaClient::HandleGetDmx(lla::rpc::SimpleRpcController *controller,
                             lla::proto::DmxData *reply) {
  string error_string;
  if (m_observer) {
    if (controller->Failed())
      error_string = controller->ErrorText();
    release_lock;
    // TODO: keep a map of registrations and filter
    m_observer->NewDmx(reply->universe(),
                       reply->data().length(),
                       (uint8_t*) reply->data().c_str(),
                       error_string);
    acquire_lock;
  }
  delete controller;
  delete reply;
}


/*
 * Called once DeviceInfo completes.
 */
void LlaClient::HandleDeviceInfo(lla::rpc::SimpleRpcController *controller,
                                 lla::proto::DeviceInfoReply *reply) {
  string error_string = "";
  vector<LlaDevice> lla_devices;

  if (m_observer) {
    if (controller->Failed())
      error_string = controller->ErrorText();
    else {
      for (int i=0; i < reply->device_size(); ++i) {
        DeviceInfo device_info = reply->device(i);

        LlaDevice device(device_info.device_id(),
                         device_info.device_name(),
                         device_info.plugin_id());

        for (int j=0; j < device_info.port_size(); ++j) {
          PortInfo port_info = device_info.port(j);
          LlaPort::PortCapability capability = port_info.output_port() ?
            LlaPort::LLA_PORT_CAP_OUT : LlaPort::LLA_PORT_CAP_IN;
          LlaPort port(port_info.port_id(),
                       capability,
                       port_info.universe(),
                       port_info.active());
          device.AddPort(port);
        }
        lla_devices.push_back(device);
      }
    }
    release_lock;
    m_observer->Devices(lla_devices, error_string);
    acquire_lock;
  }
  delete controller;
  delete reply;
}



/*
 * Called once PluginInfo completes
 */
void LlaClient::HandleUniverseInfo(SimpleRpcController *controller,
                                   lla::proto::UniverseInfoReply *reply) {
  string error_string = "";
  vector<LlaUniverse> lla_universes;

  if (m_observer) {
    if (controller->Failed())
      error_string = controller->ErrorText();
    else {
      for (int i=0; i < reply->universe_size(); ++i) {
        UniverseInfo universe_info = reply->universe(i);
        LlaUniverse::merge_mode merge_mode = (
          universe_info.merge_mode() == lla::proto::HTP ?
          LlaUniverse::MERGE_HTP: LlaUniverse::MERGE_LTP);

        LlaUniverse universe(universe_info.universe(),
                             merge_mode,
                             universe_info.name());
        lla_universes.push_back(universe);
      }
    }
    release_lock;
    m_observer->Universes(lla_universes, error_string);
    acquire_lock;
  }

  delete controller;
  delete reply;
}


/*
 * Called once SetUniverseName completes
 */
void LlaClient::HandleUniverseName(SimpleRpcController *controller,
                                   lla::proto::Ack *reply) {
  if (controller->Failed())
    printf("set name failed: %s\n", controller->ErrorText().c_str());
  delete controller;
  delete reply;
}


/*
 * Called once SetMergeMode completes
 */
void LlaClient::HandleUniverseMergeMode(SimpleRpcController *controller,
                                        lla::proto::Ack *reply) {
  if (controller->Failed())
    printf("set merge mode failed: %s\n", controller->ErrorText().c_str());
  delete controller;
  delete reply;
}


/*
 * Called once SetMergeMode completes
 */
void LlaClient::HandleRegister(SimpleRpcController *controller,
                              lla::proto::Ack *reply) {
  if (controller->Failed())
    printf("register failed: %s\n", controller->ErrorText().c_str());
  delete controller;
  delete reply;
}


/*
 * Called once SetMergeMode completes
 */
void LlaClient::HandlePatch(SimpleRpcController *controller,
                                        lla::proto::Ack *reply) {
  if (controller->Failed())
    printf("patch failed: %s\n", controller->ErrorText().c_str());
  delete controller;
  delete reply;
}


/*
 * Handle a device config response
 *
 */
void LlaClient::HandleDeviceConfig(lla::rpc::SimpleRpcController *controller,
                                   lla::proto::DeviceConfigReply *reply) {
  string error_string;
  if (m_observer) {
    if (controller->Failed())
      error_string = controller->ErrorText();

    release_lock;
    //TODO: add the device id here
    m_observer->DeviceConfig(reply->data(), error_string)
    acquire_lock;
  }
  delete controller;
  delete reply;
}


} // lla
