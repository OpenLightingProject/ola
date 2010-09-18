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
#include <string>
#include <vector>

#include "ola/BaseTypes.h"
#include "ola/Closure.h"
#include "ola/Logging.h"
#include "ola/OlaClientCore.h"
#include "ola/OlaDevice.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/RDMAPIImplInterface.h"

namespace ola {

using ola::proto::OlaServerService_Stub;
using std::string;
using std::vector;
using ola::rdm::UID;
using ola::rdm::UIDSet;

OlaClientCore::OlaClientCore(ConnectedSocket *socket)
    : m_socket(socket),
      m_dmx_callback(NULL),
      m_channel(NULL),
      m_stub(NULL),
      m_connected(false) {
}


OlaClientCore::~OlaClientCore() {
  if (m_dmx_callback) {
    delete m_dmx_callback;
    m_dmx_callback = NULL;
  }
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

  m_channel = new StreamRpcChannel(this, m_socket);

  if (!m_channel) {
    return false;
  }
  m_stub = new OlaServerService_Stub(m_channel);

  if (!m_stub) {
    delete m_channel;
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
    m_socket->Close();
    delete m_channel;
    delete m_stub;
  }
  m_connected = false;
  return 0;
}


/*
 * Fetch the list of plugins loaded.
 * @return true on success, false on failure
 */
bool OlaClientCore::FetchPluginList(
    SingleUseCallback2<void,
                       const std::vector<OlaPlugin>&,
                       const string &> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::PluginListRequest request;
  ola::proto::PluginListReply *reply = new ola::proto::PluginListReply();

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandlePluginList,
      NewArgs<plugin_list_arg>(controller, reply, callback));
  m_stub->GetPlugins(controller, &request, reply, cb);
  return true;
}


/*
 * Fetch the description for a plugin
 * @return true on success, false on failure
 */
bool OlaClientCore::FetchPluginDescription(
    ola_plugin_id plugin_id,
    SingleUseCallback2<void, const string&, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::PluginDescriptionRequest request;
  ola::proto::PluginDescriptionReply *reply = new
    ola::proto::PluginDescriptionReply();

  request.set_plugin_id(plugin_id);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandlePluginDescription,
      NewArgs<plugin_description_arg>(controller, reply, callback));
  m_stub->GetPluginDescription(controller, &request, reply, cb);
  return true;
}


/*
 * Request a listing of what devices are attached
 * @param filter only fetch devices that belong to this particular plugin
 * @return true on success, false on failure
 */
bool OlaClientCore::FetchDeviceInfo(
    ola_plugin_id filter,
    SingleUseCallback2<void,
                       const vector <class OlaDevice>&,
                       const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::DeviceInfoRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::DeviceInfoReply *reply = new ola::proto::DeviceInfoReply();
  request.set_plugin_id(filter);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleDeviceInfo,
      NewArgs<device_info_arg>(controller, reply, callback));
  m_stub->GetDeviceInfo(controller, &request, reply, cb);
  return true;
}


/*
 * Sends a device config request
 * @param device_alias the device alias
 * @param msg the data to send
 */
bool OlaClientCore::ConfigureDevice(
    unsigned int device_alias,
    const string &msg,
    SingleUseCallback2<void, const string&, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::DeviceConfigRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::DeviceConfigReply *reply = new ola::proto::DeviceConfigReply();

  string configure_request;
  request.set_device_alias(device_alias);
  request.set_data(msg);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleDeviceConfig,
      NewArgs<configure_device_args>(controller, reply, callback));
  m_stub->ConfigureDevice(controller, &request, reply, cb);
  return true;
}


/*
 * Set the priority for a port to inherit mode
 * @param dev the device id
 * @param port the port id
 * @param is_output true for an output port, false of an input port
 */
bool OlaClientCore::SetPortPriorityInherit(
    unsigned int device_alias,
    unsigned int port,
    PortDirection port_direction,
    SingleUseCallback1<void, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::PortPriorityRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_device_alias(device_alias);
  request.set_port_id(port);
  request.set_is_output(port_direction == OUTPUT_PORT);
  request.set_priority_mode(ola::PRIORITY_MODE_INHERIT);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleAck,
      NewArgs<ack_args>(controller, reply, callback));
  m_stub->SetPortPriority(controller, &request, reply, cb);
  return true;
}


/*
 * Set the priority for a port to override mode
 * @param dev the device id
 * @param port the port id
 * @param is_output true for an output port, false of an input port
 * @param value the port priority value
 */
bool OlaClientCore::SetPortPriorityOverride(
    unsigned int device_alias,
    unsigned int port,
    PortDirection port_direction,
    uint8_t value,
    SingleUseCallback1<void, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::PortPriorityRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_device_alias(device_alias);
  request.set_port_id(port);
  request.set_is_output(port_direction == OUTPUT_PORT);
  request.set_priority_mode(ola::PRIORITY_MODE_OVERRIDE);
  request.set_priority(value);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleAck,
      NewArgs<ack_args>(controller, reply, callback));
  m_stub->SetPortPriority(controller, &request, reply, cb);
  return true;
}


/*
 * Request a universe listing
 * @return true on success, false on failure
 */
bool OlaClientCore::FetchUniverseList(
    SingleUseCallback2<void,
                       const vector<class OlaUniverse>&,
                       const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::UniverseInfoRequest request;
  ola::proto::UniverseInfoReply *reply = new ola::proto::UniverseInfoReply();

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleUniverseList,
      NewArgs<universe_list_args>(controller, reply, callback));
  m_stub->GetUniverseInfo(controller, &request, reply, cb);
  return true;
}


/*
 * Fetch the information for a single universe.
 * @param universe_id the id of the universe
 * @return true on success, false on failure
 */
bool OlaClientCore::FetchUniverseInfo(
    unsigned int universe_id,
    SingleUseCallback2<void, OlaUniverse&, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::UniverseInfoRequest request;
  ola::proto::UniverseInfoReply *reply = new ola::proto::UniverseInfoReply();

  request.set_universe(universe_id);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleUniverseInfo,
      NewArgs<universe_info_args>(controller, reply, callback));
  m_stub->GetUniverseInfo(controller, &request, reply, cb);
  return true;
}


/*
 * Set the name of a universe
 * @param universe the id of the universe
 * @param name the new name
 * @return true on success, false on failure
 */
bool OlaClientCore::SetUniverseName(
    unsigned int universe,
    const string &name,
    SingleUseCallback1<void, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::UniverseNameRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_universe(universe);
  request.set_name(name);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleAck,
      NewArgs<ack_args>(controller, reply, callback));
  m_stub->SetUniverseName(controller, &request, reply, cb);
  return true;
}


/*
 * Set the merge mode of a universe
 * @param universe the id of the universe
 * @param mode the new merge mode
 * @return true on success, false on failure
 */
bool OlaClientCore::SetUniverseMergeMode(
    unsigned int universe,
    OlaUniverse::merge_mode mode,
    SingleUseCallback1<void, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::MergeModeRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  ola::proto::MergeMode merge_mode = mode == OlaUniverse::MERGE_HTP ?
    ola::proto::HTP : ola::proto::LTP;
  request.set_universe(universe);
  request.set_merge_mode(merge_mode);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleAck,
      NewArgs<ack_args>(controller, reply, callback));
  m_stub->SetMergeMode(controller, &request, reply, cb);
  return true;
}


/*
 * (Un)Patch a port to a universe
 * @param device_alias the alias of the device
 * @param port  the port id
 * @param action OlaClientCore::PATCH or OlaClientCore::UNPATCH
 * @param universe universe id
 */
bool OlaClientCore::Patch(
    unsigned int device_alias,
    unsigned int port_id,
    ola::PortDirection port_direction,
    ola::PatchAction patch_action,
    unsigned int universe,
    SingleUseCallback1<void, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::PatchPortRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  ola::proto::PatchAction action = (
      patch_action == ola::PATCH ? ola::proto::PATCH : ola::proto::UNPATCH);
  request.set_universe(universe);
  request.set_device_alias(device_alias);
  request.set_port_id(port_id);
  request.set_is_output(port_direction == OUTPUT_PORT);
  request.set_action(action);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleAck,
      NewArgs<ack_args>(controller, reply, callback));
  m_stub->PatchPort(controller, &request, reply, cb);
  return true;
}


/*
 * Set the callback to be run when DMX data arrives
 */
void OlaClientCore::SetDmxCallback(
    Callback3<void, unsigned int, const DmxBuffer&, const string&> *callback) {
  if (m_dmx_callback)
    delete m_dmx_callback;
  m_dmx_callback = callback;
}


/*
 * Register our interest in a universe, the observer object will then
 * be notifed when the dmx values in this universe change.
 * @param universe the id of the universe
 * @param action the action (register or unregister)
 */
bool OlaClientCore::RegisterUniverse(
    unsigned int universe,
    ola::RegisterAction register_action,
    SingleUseCallback1<void, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::RegisterDmxRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  ola::proto::RegisterAction action = (
      register_action == ola::REGISTER ? ola::proto::REGISTER :
        ola::proto::UNREGISTER);
  request.set_universe(universe);
  request.set_action(action);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleAck,
      NewArgs<ack_args>(controller, reply, callback));
  m_stub->RegisterForDmx(controller, &request, reply, cb);
  return true;
}


/*
 * Write some dmx data
 * @param universe   universe to send to
 * @param data the DmxBuffer with the data
 * @return true on success, false on failure
 */
bool OlaClientCore::SendDmx(
    unsigned int universe,
    const DmxBuffer &data,
    SingleUseCallback1<void, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::DmxData request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_universe(universe);
  request.set_data(data.Get());

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleAck,
      NewArgs<ack_args>(controller, reply, callback));
  m_stub->UpdateDmxData(controller, &request, reply, cb);
  return true;
}


/*
 * Read dmx data
 * @param universe the universe id to get data for
 * @return true on success, false on failure
 */
bool OlaClientCore::FetchDmx(
    unsigned int universe,
    SingleUseCallback2<void, const DmxBuffer&, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::DmxReadRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::DmxData *reply = new ola::proto::DmxData();

  request.set_universe(universe);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleGetDmx,
      NewArgs<get_dmx_args>(controller, reply, callback));
  m_stub->GetDmx(controller, &request, reply, cb);
  return true;
}


/*
 * Fetch the UID list for a universe
 */
bool OlaClientCore::FetchUIDList(
    unsigned int universe,
    SingleUseCallback2<void,
                       const ola::rdm::UIDSet&,
                       const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::UniverseRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::UIDListReply *reply = new ola::proto::UIDListReply();

  request.set_universe(universe);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleUIDList,
      NewArgs<uid_list_args>(controller, reply, callback));
  m_stub->GetUIDs(controller, &request, reply, cb);
  return true;
}


/*
 * Force RDM discovery for a universe
 * @param universe the universe id to run discovery on
 * @return true on success, false on failure
 */
bool OlaClientCore::ForceDiscovery(
    unsigned int universe,
    ola::SingleUseCallback1<void, const string&> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::UniverseRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_universe(universe);

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleAck,
      NewArgs<ack_args>(controller, reply, callback));
  m_stub->ForceDiscovery(controller, &request, reply, cb);
  return true;
}


/*
 * Set this clients Source UID
 */
bool OlaClientCore::SetSourceUID(
    const UID &uid,
    ola::SingleUseCallback1<void, const string &> *callback) {
  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::UID request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_esta_id(uid.ManufacturerId());
  request.set_device_id(uid.DeviceId());

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleAck,
      NewArgs<ack_args>(controller, reply, callback));
  m_stub->SetSourceUID(controller, &request, reply, cb);
  return true;
}


/*
 * Send an RDM Get Command
 * @param callback the Callback to invoke when this completes
 * @param universe the universe to send the command on
 * @param uid the UID to send the command to
 * @param sub_device the sub device index
 * @param pid the PID to address
 * @param data the optional data to send
 * @param data_length the length of the data
 * @return true on success, false on failure
 */
bool OlaClientCore::RDMGet(
    ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint16_t pid,
    const uint8_t *data,
    unsigned int data_length) {
  return RDMCommand(callback, false, universe, uid, sub_device, pid, data,
                    data_length);
}


/*
 * Send an RDM Set Command
 * @param callback the Callback to invoke when this completes
 * @param universe the universe to send the command on
 * @param uid the UID to send the command to
 * @param sub_device the sub device index
 * @param pid the PID to address
 * @param data the optional data to send
 * @param data_length the length of the data
 * @return true on success, false on failure
 */
bool OlaClientCore::RDMSet(
    ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint16_t pid,
    const uint8_t *data,
    unsigned int data_length) {
  return RDMCommand(callback, true, universe, uid, sub_device, pid, data,
                    data_length);
}


/*
 * Called when new DMX data arrives
 */
void OlaClientCore::UpdateDmxData(
    ::google::protobuf::RpcController *controller,
    const ola::proto::DmxData *request,
    ola::proto::Ack *response,
    ::google::protobuf::Closure *done) {
  if (m_dmx_callback) {
    DmxBuffer buffer;
    buffer.Set(request->data());
    m_dmx_callback->Run(request->universe(), buffer, "");
  }
  done->Run();
  (void) response;
  (void) controller;
}


// The following are RPC callbacks

/*
 * Called once PluginInfo completes
 */
void OlaClientCore::HandlePluginList(plugin_list_arg *args) {
  string error_string = "";
  vector<OlaPlugin> ola_plugins;

  if (!args->callback) {
    FreeArgs(args);
    return;
  }

  if (args->controller->Failed()) {
    error_string = args->controller->ErrorText();
  } else {
    for (int i = 0; i < args->reply->plugin_size(); ++i) {
      ola::proto::PluginInfo plugin_info = args->reply->plugin(i);
      OlaPlugin plugin(plugin_info.plugin_id(),
                       plugin_info.name());
      ola_plugins.push_back(plugin);
    }
  }
  std::sort(ola_plugins.begin(), ola_plugins.end());
  args->callback->Run(ola_plugins, error_string);
  FreeArgs(args);
}


/*
 * Called once PluginInfo completes
 */
void OlaClientCore::HandlePluginDescription(plugin_description_arg *args) {
  string error_string = "";
  string description;

  if (!args->callback) {
    FreeArgs(args);
    return;
  }

  if (args->controller->Failed()) {
    error_string = args->controller->ErrorText();
  }
  args->callback->Run(args->reply->description(), error_string);
  FreeArgs(args);
}


/*
 * Called once DeviceInfo completes.
 */
void OlaClientCore::HandleDeviceInfo(device_info_arg *args) {
  string error_string = "";
  vector<OlaDevice> ola_devices;

  if (!args->callback) {
    FreeArgs(args);
    return;
  }

  if (args->controller->Failed()) {
    error_string = args->controller->ErrorText();
  } else {
    for (int i = 0; i < args->reply->device_size(); ++i) {
      ola::proto::DeviceInfo device_info = args->reply->device(i);
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
            port_info.priority());
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
            port_info.priority());
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
  args->callback->Run(ola_devices, error_string);
  FreeArgs(args);
}


/*
 * Handle a device config response
 */
void OlaClientCore::HandleDeviceConfig(configure_device_args *args) {
  string error_string;
  if (!args->callback) {
    FreeArgs(args);
    return;
  }

  if (args->controller->Failed())
    error_string = args->controller->ErrorText();

  args->callback->Run(args->reply->data(), error_string);
  FreeArgs(args);
}


/*
 * Called once SetPriority completes
 */
void OlaClientCore::HandleAck(ack_args *args) {
  string error_string = "";
  if (!args->callback) {
    FreeArgs(args);
    return;
  }

  if (args->controller->Failed())
    error_string = args->controller->ErrorText();
  args->callback->Run(error_string);
  FreeArgs(args);
}


/*
 * Called once UniverseInfo completes
 */
void OlaClientCore::HandleUniverseList(universe_list_args *args) {
  string error_string = "";
  vector<OlaUniverse> ola_universes;

  if (!args->callback) {
    FreeArgs(args);
    return;
  }

  if (args->controller->Failed()) {
    error_string = args->controller->ErrorText();
  } else {
    for (int i = 0; i < args->reply->universe_size(); ++i) {
      ola::proto::UniverseInfo universe_info = args->reply->universe(i);
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
  args->callback->Run(ola_universes, error_string);
  FreeArgs(args);
}


/*
 * Called once UniverseInfo completes
 */
void OlaClientCore::HandleUniverseInfo(universe_info_args *args) {
  string error_string = "";
  OlaUniverse null_universe(0,
                            OlaUniverse::MERGE_LTP,
                            "",
                            0,
                            0,
                            0);

  if (!args->callback) {
    FreeArgs(args);
    return;
  }

  if (args->controller->Failed()) {
    error_string = args->controller->ErrorText();
  } else {
    if (args->reply->universe_size() == 1) {
      ola::proto::UniverseInfo universe_info = args->reply->universe(0);
      OlaUniverse::merge_mode merge_mode = (
        universe_info.merge_mode() == ola::proto::HTP ?
        OlaUniverse::MERGE_HTP: OlaUniverse::MERGE_LTP);

      OlaUniverse universe(universe_info.universe(),
                           merge_mode,
                           universe_info.name(),
                           universe_info.input_port_count(),
                           universe_info.output_port_count(),
                           universe_info.rdm_devices());
      args->callback->Run(universe, error_string);
      FreeArgs(args);
      return;

    } else if (args->reply->universe_size() > 1) {
      error_string = "Too many unvierses in response";
    } else {
      error_string = "Universe not found";
    }
  }
  args->callback->Run(null_universe, error_string);
  FreeArgs(args);
}


/*
 * Called once GetDmx completes
 */
void OlaClientCore::HandleGetDmx(get_dmx_args *args) {
  string error_string;
  if (!args->callback) {
    FreeArgs(args);
    return;
  }
  if (args->controller->Failed())
    error_string = args->controller->ErrorText();

  DmxBuffer buffer;
  buffer.Set(args->reply->data());
  args->callback->Run(buffer, error_string);
  FreeArgs(args);
}


/*
 * Handle the UID list response
 */
void OlaClientCore::HandleUIDList(uid_list_args *args) {
  string error_string = "";
  ola::rdm::UIDSet uid_set;

  if (!args->callback) {
    FreeArgs(args);
    return;
  }

  if (args->controller->Failed()) {
    error_string = args->controller->ErrorText();
  } else {
    for (int i = 0; i < args->reply->uid_size(); ++i) {
      ola::proto::UID proto_uid = args->reply->uid(i);
      ola::rdm::UID uid(proto_uid.esta_id(),
                        proto_uid.device_id());
      uid_set.AddUID(uid);
    }
  }
  args->callback->Run(uid_set, error_string);
  FreeArgs(args);
}


/*
 * Handle an RDM message
 */
void OlaClientCore::HandleRDM(rdm_response_args *args) {
  ola::rdm::RDMAPIImplResponseStatus response_status;

  if (!args->callback) {
    FreeArgs(args);
    return;
  }

  response_status.was_broadcast = args->reply->was_broadcast();
  response_status.response_type = args->reply->response_code();
  response_status.message_count = args->reply->message_count();

  if (args->controller->Failed()) {
    response_status.error = args->controller->ErrorText();
  } else if (args->reply->response_code() == ola::rdm::ACK_OVERFLOW ||
             args->reply->response_code() == ola::rdm::ACK_TIMER) {
    // TODO(simon): handle to ACK_OVERFLOW and ACK_TIMER cases here or
    // preferably in the ola server.
    OLA_WARN << "We don't handle ACK_OVERFLOW or ACK_TIMER yet!";
    response_status.error = "OVERFLOW or ACK not implemented in OLA Client";
  }

  args->callback->Run(response_status, args->reply->data());
  FreeArgs(args);
}


/*
 * Send a generic rdm command
 */
bool OlaClientCore::RDMCommand(
    ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
    bool is_set,
    unsigned int universe,
    const UID &uid,
    uint16_t sub_device,
    uint16_t pid,
    const uint8_t *data,
    unsigned int data_length) {
  if (!callback) {
    OLA_WARN << "RDM callback was null, command to " << uid << " won't be sent";
    return false;
  }

  if (!m_connected) {
    delete callback;
    return false;
  }

  ola::proto::RDMRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::RDMResponse *reply = new ola::proto::RDMResponse();

  request.set_universe(universe);
  ola::proto::UID *pb_uid = request.mutable_uid();
  pb_uid->set_esta_id(uid.ManufacturerId());
  pb_uid->set_device_id(uid.DeviceId());
  request.set_sub_device(sub_device);
  request.set_param_id(pid);
  request.set_is_set(is_set);
  request.set_data(string(reinterpret_cast<const char*>(data), data_length));

  google::protobuf::Closure *cb = google::protobuf::NewCallback(
      this,
      &ola::OlaClientCore::HandleRDM,
      NewArgs<rdm_response_args>(controller, reply, callback));

  m_stub->RDMCommand(controller, &request, reply, cb);
  return true;
}
}  // ola
