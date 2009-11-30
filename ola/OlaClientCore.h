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

#ifdef OLA_HAVE_PTHREAD
#include <pthread.h>
#endif

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
    // int send_rdm(int universe, uint8_t *data, int length);
    bool SetUniverseName(unsigned int uni, const string &name);
    bool SetUniverseMergeMode(unsigned int uni, OlaUniverse::merge_mode mode);

    bool RegisterUniverse(unsigned int universe, ola::RegisterAction action);

    bool Patch(unsigned int device_alias,
               unsigned int port,
               bool is_output,
               ola::PatchAction action,
               unsigned int uni);

    bool ConfigureDevice(unsigned int device_alias, const string &msg);

    // request callbacks
    void HandlePluginInfo(SimpleRpcController *controller,
                          ola::proto::PluginInfoReply *reply);
    void HandleSendDmx(SimpleRpcController *controller,
                       ola::proto::Ack *reply);
    void HandleGetDmx(SimpleRpcController *controller,
                      ola::proto::DmxData *reply);
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
    void HandleDeviceConfig(SimpleRpcController *controller,
                            ola::proto::DeviceConfigReply *reply);

  private:
    OlaClientCore(const OlaClientCore&);
    OlaClientCore operator=(const OlaClientCore&);

    ConnectedSocket *m_socket;
    OlaClientServiceImpl *m_client_service;
    StreamRpcChannel *m_channel;
    ola::proto::OlaServerService_Stub *m_stub;
    int m_connected;
    OlaClientObserver *m_observer;
#ifdef OLA_HAVE_PTHREAD
    pthread_mutex_t m_mutex;
#endif
};
}  // ola
#endif  // OLA_OLACLIENTCORE_H_
