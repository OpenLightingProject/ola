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
 * OlaClientCore.h
 * The OLA Client Core class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef OLA_OLACLIENTCORE_H_
#define OLA_OLACLIENTCORE_H_

#include <google/protobuf/stubs/common.h>
#include <string>

#include "common/protocol/Ola.pb.h"
#include "common/rpc/SimpleRpcController.h"
#include "common/rpc/StreamRpcChannel.h"
#include "ola/DmxBuffer.h"
#include "ola/OlaClient.h"
#include "ola/OlaClientServiceImpl.h"
#include "ola/OlaDevice.h"
#include "ola/common.h"
#include "ola/network/Socket.h"
#include "ola/plugin_id.h"
#include "ola/rdm/RDMAPIImplInterface.h"

namespace ola {

class OlaClientCoreServiceImpl;

using std::string;
using ola::network::ConnectedSocket;
using ola::rpc::SimpleRpcController;
using ola::rpc::StreamRpcChannel;

class OlaClientCore {
  public:
    explicit OlaClientCore(ConnectedSocket *socket);
    ~OlaClientCore();

    bool Setup();
    bool Stop();
    bool SetObserver(OlaClientObserver *observer);

    bool FetchPluginInfo(ola_plugin_id filter, bool include_description);
    bool FetchDeviceInfo(ola_plugin_id filter);
    bool FetchUniverseInfo();

    // dmx methods
    bool SendDmx(unsigned int universe, const DmxBuffer &data);
    bool FetchDmx(unsigned int uni);

    // rdm methods
    bool FetchUIDList(unsigned int universe);
    bool ForceDiscovery(unsigned int universe);
    bool SetSourceUID(const UID &uid,
                      ola::SingleUseCallback1<void, const string &> *callback);

    bool RDMGet(ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
                unsigned int universe,
                const UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length);
    bool RDMSet(ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
                unsigned int universe,
                const UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length);

    bool SetUniverseName(unsigned int uni, const string &name);
    bool SetUniverseMergeMode(unsigned int uni, OlaUniverse::merge_mode mode);

    bool RegisterUniverse(unsigned int universe, ola::RegisterAction action);

    bool Patch(unsigned int device_alias,
               unsigned int port,
               bool is_output,
               ola::PatchAction action,
               unsigned int uni);

    bool SetPortPriorityInherit(unsigned int device_alias,
                                unsigned int port,
                                bool is_output);
    bool SetPortPriorityOverride(unsigned int device_alias,
                                 unsigned int port,
                                 bool is_output,
                                 uint8_t value);

    bool ConfigureDevice(unsigned int device_alias, const string &msg);

    // request callbacks
    void HandlePluginInfo(SimpleRpcController *controller,
                          ola::proto::PluginInfoReply *reply);
    void HandleSendDmx(SimpleRpcController *controller,
                       ola::proto::Ack *reply);
    void HandleGetDmx(SimpleRpcController *controller,
                      ola::proto::DmxData *reply);
    void HandleUIDList(SimpleRpcController *controller,
                       ola::proto::UIDListReply *reply);
    void HandleDeviceInfo(SimpleRpcController *controller,
                          ola::proto::DeviceInfoReply *reply);
    void HandleUniverseInfo(SimpleRpcController *controller,
                            ola::proto::UniverseInfoReply *reply);
    void HandleUniverseName(SimpleRpcController *controller,
                            ola::proto::Ack *reply);
    void HandleUniverseMergeMode(SimpleRpcController *controller,
                                 ola::proto::Ack *reply);
    void HandleRegister(SimpleRpcController *controller,
                        ola::proto::Ack *reply);
    void HandlePatch(SimpleRpcController *controller,
                     ola::proto::Ack *reply);
    void HandleSetPriority(SimpleRpcController *controller,
                           ola::proto::Ack *reply);
    void HandleDeviceConfig(SimpleRpcController *controller,
                            ola::proto::DeviceConfigReply *reply);
    void HandleDiscovery(SimpleRpcController *controller,
                         ola::proto::UniverseAck *reply);

    // we need these because a google::protobuf::Closure can't take more than 2
    // args
    struct rdm_response_args {
      ola::rdm::RDMAPIImplInterface::rdm_callback *callback;
      SimpleRpcController *controller;
      ola::proto::RDMResponse *reply;
    };

    struct set_source_uid_args {
      ola::SingleUseCallback1<void, const string &> *callback;
      SimpleRpcController *controller;
      ola::proto::Ack *reply;
    };

    void HandleRDM(struct rdm_response_args *args);
    void HandleSetSourceUID(struct set_source_uid_args *args);

  private:
    OlaClientCore(const OlaClientCore&);
    OlaClientCore operator=(const OlaClientCore&);

    bool RDMCommand(ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
                    bool is_set,
                    unsigned int universe,
                    const UID &uid,
                    uint16_t sub_device,
                    uint16_t pid,
                    const uint8_t *data,
                    unsigned int data_length);

    ConnectedSocket *m_socket;
    OlaClientServiceImpl *m_client_service;
    StreamRpcChannel *m_channel;
    ola::proto::OlaServerService_Stub *m_stub;
    int m_connected;
    OlaClientObserver *m_observer;
};
}  // ola
#endif  // OLA_OLACLIENTCORE_H_
