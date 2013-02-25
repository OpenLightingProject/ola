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
#include <vector>

#include "common/protocol/Ola.pb.h"
#include "common/rpc/SimpleRpcController.h"
#include "common/rpc/StreamRpcChannel.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/OlaCallbackClient.h"
#include "ola/OlaDevice.h"
#include "ola/common.h"
#include "ola/network/Socket.h"
#include "ola/plugin_id.h"
#include "ola/rdm/RDMAPIImplInterface.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/timecode/TimeCode.h"

namespace ola {

class OlaClientCoreServiceImpl;

using std::string;
using ola::io::ConnectedDescriptor;
using ola::rpc::SimpleRpcController;
using ola::rpc::StreamRpcChannel;

class OlaClientCore: public ola::proto::OlaClientService {
  public:
    explicit OlaClientCore(ConnectedDescriptor *descriptor);
    ~OlaClientCore();

    bool Setup();
    bool Stop();

    // plugin methods
    bool FetchPluginList(
        SingleUseCallback2<void,
                           const vector<class OlaPlugin>&,
                           const string&> *callback);

    bool FetchPluginDescription(
        ola_plugin_id plugin_id,
        SingleUseCallback2<void, const string&, const string&> *callback);

    bool FetchPluginState(ola_plugin_id plugin_id,
                          OlaCallbackClient::PluginStateCallback *callback);

    // device methods
    bool FetchDeviceInfo(
        ola_plugin_id filter,
        SingleUseCallback2<void,
                           const vector <class OlaDevice>&,
                           const string&> *callback);

    bool FetchCandidatePorts(
        SingleUseCallback2<void,
                           const vector <class OlaDevice>&,
                           const string&> *callback);

    bool FetchCandidatePorts(
        unsigned int universe_id,
        SingleUseCallback2<void,
                           const vector <class OlaDevice>&,
                           const string&> *callback);

    bool ConfigureDevice(
        unsigned int device_alias,
        const string &msg,
        SingleUseCallback2<void, const string&, const string&> *callback);

    // port methods
    bool SetPortPriorityInherit(
        unsigned int device_alias,
        unsigned int port,
        PortDirection port_direction,
        SingleUseCallback1<void, const string&> *callback);
    bool SetPortPriorityOverride(
        unsigned int device_alias,
        unsigned int port,
        PortDirection port_direction,
        uint8_t value,
        SingleUseCallback1<void, const string&> *callback);

    // universe methods
    bool FetchUniverseList(
        SingleUseCallback2<void,
                           const vector <class OlaUniverse>&,
                           const string &> *callback);
    bool FetchUniverseInfo(
        unsigned int universe,
        SingleUseCallback2<void,
                           class OlaUniverse&,
                           const string &> *callback);
    bool SetUniverseName(
        unsigned int uni,
        const string &name,
        SingleUseCallback1<void, const string&> *callback);
    bool SetUniverseMergeMode(
        unsigned int uni,
        OlaUniverse::merge_mode mode,
        SingleUseCallback1<void, const string&> *callback);

    // patching
    bool Patch(
        unsigned int device_alias,
        unsigned int port,
        ola::PortDirection port_direction,
        ola::PatchAction action,
        unsigned int uni,
        SingleUseCallback1<void, const string&> *callback);

    // dmx methods
    void SetDmxCallback(
        Callback3<void,
                  unsigned int,
                  const DmxBuffer&, const string&> *callback);

    bool RegisterUniverse(
        unsigned int universe,
        ola::RegisterAction register_action,
        SingleUseCallback1<void, const string&> *callback);
    bool SendDmx(
        unsigned int universe,
        const DmxBuffer &data,
        SingleUseCallback1<void, const string&> *callback);
    bool SendDmx(
        unsigned int universe,
        const DmxBuffer &data,
        Callback1<void, const string&> *callback);
    bool SendDmx(unsigned int universe, const DmxBuffer &data);
    bool FetchDmx(
        unsigned int universe,
        SingleUseCallback2<void, const DmxBuffer&, const string&> *callback);

    // rdm methods
    bool FetchUIDList(
        unsigned int universe,
        SingleUseCallback2<void,
                           const ola::rdm::UIDSet&,
                           const string&> *callback);
    bool RunDiscovery(
        unsigned int universe,
        bool full,
        ola::SingleUseCallback2<void,
                                const ola::rdm::UIDSet&,
                                const string&> *callback);
    bool SetSourceUID(const ola::rdm::UID &uid,
                      ola::SingleUseCallback1<void, const string&> *callback);

    bool RDMGet(ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
                unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length);
    bool RDMGet(ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback,
                unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length);
    bool RDMSet(ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
                unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length);
    bool RDMSet(ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback,
                unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length);

    // timecode
    bool SendTimeCode(ola::SingleUseCallback1<void, const string&> *callback,
                      const ola::timecode::TimeCode &timecode);

    /*
     * This is called by the channel when new DMX data turns up
     */
    void UpdateDmxData(::google::protobuf::RpcController* controller,
                       const ola::proto::DmxData* request,
                       ola::proto::Ack* response,
                       ::google::protobuf::Closure* done);

    // unfortunately all of these need to be public because they're used in the
    // closures. That's why this class is wrapped in OlaClient or
    // OlaCallbackClient.
    typedef struct {
      SimpleRpcController *controller;
      ola::proto::PluginListReply *reply;
      SingleUseCallback2<void,
                         const vector<class OlaPlugin>&,
                         const string&> *callback;
    } plugin_list_arg;

    void HandlePluginList(plugin_list_arg *args);

    typedef struct {
      SimpleRpcController *controller;
      ola::proto::PluginDescriptionReply *reply;
      SingleUseCallback2<void, const string&, const string&> *callback;
    } plugin_description_arg;

    void HandlePluginDescription(plugin_description_arg *args);

    typedef struct {
      SimpleRpcController *controller;
      ola::proto::PluginStateReply *reply;
      OlaCallbackClient::PluginStateCallback *callback;
    } plugin_state_arg;

    void HandlePluginState(plugin_state_arg *args);

    typedef struct {
      SimpleRpcController *controller;
      ola::proto::DeviceInfoReply *reply;
      SingleUseCallback2<void,
                         const vector <class OlaDevice> &,
                         const string &> *callback;
    } device_info_arg;

    void HandleDeviceInfo(device_info_arg *arg);

    typedef struct {
      SimpleRpcController *controller;
      ola::proto::DeviceConfigReply *reply;
      SingleUseCallback2<void, const string&, const string&> *callback;
    } configure_device_args;

    void HandleDeviceConfig(configure_device_args *arg);

    typedef struct {
      SimpleRpcController *controller;
      ola::proto::Ack *reply;
      BaseCallback1<void, const string&> *callback;
    } ack_args;

    void HandleAck(ack_args *args);

    typedef struct {
      SimpleRpcController *controller;
      ola::proto::UniverseInfoReply *reply;
      SingleUseCallback2<void,
                         const vector <class OlaUniverse>&,
                         const string&> *callback;
    } universe_list_args;

    void HandleUniverseList(universe_list_args *args);

    typedef struct {
      SimpleRpcController *controller;
      ola::proto::UniverseInfoReply *reply;
      SingleUseCallback2<void, class OlaUniverse&, const string&> *callback;
    } universe_info_args;

    void HandleUniverseInfo(universe_info_args *args);

    typedef struct {
      SimpleRpcController *controller;
      ola::proto::DmxData *reply;
      SingleUseCallback2<void, const DmxBuffer&, const string&> *callback;
    } get_dmx_args;

    void HandleGetDmx(get_dmx_args *args);

    typedef struct {
      SimpleRpcController *controller;
      ola::proto::UIDListReply *reply;
      SingleUseCallback2<void,
                         const ola::rdm::UIDSet&,
                         const string&> *callback;
    } uid_list_args;

    void HandleUIDList(uid_list_args *args);

    typedef struct {
      ola::rdm::RDMAPIImplInterface::rdm_callback *callback;
      SimpleRpcController *controller;
      ola::proto::RDMResponse *reply;
    } rdm_response_args;

    void HandleRDM(rdm_response_args *args);

    typedef struct {
      ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback;
      SimpleRpcController *controller;
      ola::proto::RDMResponse *reply;
    } rdm_pid_response_args;

    void HandleRDMWithPID(rdm_pid_response_args *args);

  private:
    OlaClientCore(const OlaClientCore&);
    OlaClientCore operator=(const OlaClientCore&);

    bool GenericSendDmx(
        unsigned int universe,
        const DmxBuffer &data,
        BaseCallback1<void, const string&> *callback);

    bool GenericFetchCandidatePorts(
        unsigned int universe_id,
        bool include_universe,
        SingleUseCallback2<void,
                           const vector <class OlaDevice>&,
                           const string&> *callback);

    bool RDMCommand(ola::rdm::RDMAPIImplInterface::rdm_callback *callback,
                    bool is_set,
                    unsigned int universe,
                    const ola::rdm::UID &uid,
                    uint16_t sub_device,
                    uint16_t pid,
                    const uint8_t *data,
                    unsigned int data_length);
    bool RDMCommandWithPid(
        ola::rdm::RDMAPIImplInterface::rdm_pid_callback *callback,
        bool is_set,
        unsigned int universe,
        const ola::rdm::UID &uid,
        uint16_t sub_device,
        uint16_t pid,
        const uint8_t *data,
        unsigned int data_length);

    void CheckRDMResponseStatus(SimpleRpcController *controller,
                                ola::proto::RDMResponse *reply,
                                ola::rdm::ResponseStatus *new_status);

    void GetParamFromReply(
        const string &message_type,
        ola::proto::RDMResponse *reply,
        ola::rdm::ResponseStatus *new_status);

    void UpdateResponseAckData(
        ola::proto::RDMResponse *reply,
        ola::rdm::ResponseStatus *new_status);

    template <typename arg_type, typename reply_type, typename callback_type>
    arg_type *NewArgs(
        SimpleRpcController *controller,
        reply_type reply,
        callback_type callback);

    template <typename arg_type>
    void FreeArgs(arg_type *args);

    ConnectedDescriptor *m_descriptor;
    Callback3<void, unsigned int, const DmxBuffer&, const string&>
      *m_dmx_callback;
    StreamRpcChannel *m_channel;
    ola::proto::OlaServerService_Stub *m_stub;
    int m_connected;
};


/**
 * Create a new args structure
 */
template <typename arg_type, typename reply_type, typename callback_type>
arg_type *OlaClientCore::NewArgs(
    SimpleRpcController *controller,
    reply_type reply,
    callback_type callback) {
  arg_type *args = new arg_type();
  args->controller = controller;
  args->reply = reply;
  args->callback = callback;
  return args;
}


/**
 * Free an args structure
 */
template <typename arg_type>
void OlaClientCore::FreeArgs(arg_type *args) {
  delete args->controller;
  delete args->reply;
  delete args;
}
}  // ola
#endif  // OLA_OLACLIENTCORE_H_
