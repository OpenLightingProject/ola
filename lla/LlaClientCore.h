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
 * LlaClientCore.h
 * The LLA Client Core class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_CLIENT_CORE_H
#define LLA_CLIENT_CORE_H

#ifdef LLA_HAVE_PTHREAD
#include <pthread.h>
#endif

#include <string>

#include <google/protobuf/stubs/common.h>
#include <lla/select_server/Socket.h>
#include <lla/common.h>
#include <lla/plugin_id.h>
#include <lla/LlaDevice.h>
#include <lla/LlaClient.h>

#include "common/protocol/Lla.pb.h"
#include "common/rpc/StreamRpcChannel.h"
#include "common/rpc/SimpleRpcController.h"

#include "common/protocol/Lla.pb.h"
#include "LlaClientServiceImpl.h"

namespace lla {

class LlaClientCoreServiceImpl;

using std::string;
using google::protobuf::Closure;
using lla::select_server::ConnectedSocket;
using lla::rpc::SimpleRpcController;
using lla::rpc::StreamRpcChannel;

class LlaClientCore {
  public:
    LlaClientCore(ConnectedSocket *socket);
    ~LlaClientCore();

    bool Setup();
    bool Stop();
    bool SetObserver(LlaClientObserver *observer);

    bool FetchPluginInfo(int plugin_id, bool include_description);
    bool FetchDeviceInfo(lla_plugin_id filter);
    bool FetchUniverseInfo();

    // dmx methods
    bool SendDmx(unsigned int universe, dmx_t *data, unsigned int length);
    bool FetchDmx(unsigned int uni);

    // rdm methods
    // int send_rdm(int universe, uint8_t *data, int length);
    bool SetUniverseName(unsigned int uni, const string &name);
    bool SetUniverseMergeMode(unsigned int uni, LlaUniverse::merge_mode mode);

    bool RegisterUniverse(unsigned int universe, lla::RegisterAction action);

    bool Patch(unsigned int device,
              unsigned int port,
              lla::PatchAction action,
              unsigned int uni);

    bool ConfigureDevice(unsigned int device_id, const string &msg);

    // request callbacks
    void HandlePluginInfo(SimpleRpcController *controller,
                          lla::proto::PluginInfoReply *reply);
    void HandleSendDmx(SimpleRpcController *controller,
                       lla::proto::Ack *reply);
    void HandleGetDmx(SimpleRpcController *controller,
                      lla::proto::DmxData *reply);
    void HandleDeviceInfo(SimpleRpcController *controller,
                          lla::proto::DeviceInfoReply *reply);
    void HandleUniverseInfo(SimpleRpcController *controller,
                            lla::proto::UniverseInfoReply *reply);
    void HandleUniverseName(SimpleRpcController *controller,
                            lla::proto::Ack *reply);
    void HandleUniverseMergeMode(SimpleRpcController *controller,
                                 lla::proto::Ack *reply);
    void HandleRegister(SimpleRpcController *controller,
                        lla::proto::Ack *reply);
    void HandlePatch(SimpleRpcController *controller,
                     lla::proto::Ack *reply);
    void HandleDeviceConfig(SimpleRpcController *controller,
                            lla::proto::DeviceConfigReply *reply);

  private:
    LlaClientCore(const LlaClientCore&);
    LlaClientCore operator=(const LlaClientCore&);

    ConnectedSocket *m_socket;
    LlaClientServiceImpl *m_client_service;
    StreamRpcChannel *m_channel;
    lla::proto::LlaServerService_Stub *m_stub;

    // instance vars
#ifdef LLA_HAVE_PTHREAD
    pthread_mutex_t m_mutex;
#endif

    int m_connected;
    LlaClientObserver *m_observer;
};

} // lla
#endif
