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
 * LlaClient.h
 * Interface to the LLA Client class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_CLIENT_H
#define LLA_CLIENT_H

#ifdef LLA_HAVE_PTHREAD
#include <pthread.h>
#endif

#include <string>
#include <vector>
#include <stdint.h>

#include <lla/common.h>
#include <lla/plugin_id.h>
#include <lla/messages.h>
#include <lla/LlaDevice.h>

#include "common/protocol/Lla.pb.h"

namespace google {
namespace protobuf {
class Closure;
}
}

namespace lla {

class LlaClientServiceImpl;

namespace proto {
class LlaServerService_Stub;
}

namespace rpc {
class StreamRpcChannel;
class SimpleRpcController;
}

namespace select_server {
class ConnectedSocket;
}


class LlaClientObserver {
  public:
    virtual ~LlaClientObserver() {}

    virtual int NewDmx(unsigned int universe,
                       unsigned int length,
                       uint8_t *data,
                       const string &error) { return 0; }
    virtual int Plugins(const std::vector <class LlaPlugin> &plugins,
                        const string &error) { return 0; }
    virtual int Devices(const std::vector <class LlaDevice> devices,
                        const string &error) { return 0; }
    virtual int Universes(const std::vector <class LlaUniverse> universes,
                          const string &error) { return 0; }
    virtual int PluginDescription(class LlaPlugin *plug) { return 0; }
    virtual int DeviceConfig(const string &reply,
                             const string &error) { return 0; }
};


class LlaClient {
  public:
    enum PatchAction {PATCH, UNPATCH};
    enum RegisterAction {REGISTER, UNREGISTER};

    LlaClient(lla::select_server::ConnectedSocket *socket);
    ~LlaClient();

    bool Setup();
    bool Stop();
    int SetObserver(LlaClientObserver *o);

    bool FetchPluginInfo(int plugin_id=-1, bool include_description=false);
    int FetchDeviceInfo(lla_plugin_id filter=LLA_PLUGIN_ALL);
    int FetchUniverseInfo();

    // dmx methods
    bool SendDmx(unsigned int universe, uint8_t *data, unsigned int length);
    int FetchDmx(unsigned int uni);

    // rdm methods
    // int send_rdm(int universe, uint8_t *data, int length);
    int SetUniverseName(unsigned int uni, const std::string &name);
    int SetUniverseMergeMode(unsigned int uni, LlaUniverse::merge_mode mode);

    int RegisterUniverse(unsigned int universe, LlaClient::RegisterAction action);

    int Patch(unsigned int device,
              unsigned int port,
              LlaClient::PatchAction action,
              unsigned int uni);

    int ConfigureDevice(unsigned int dev, const string &msg);

    // request callbacks
    void HandlePluginInfo(lla::rpc::SimpleRpcController *controller,
                          lla::proto::PluginInfoReply *reply);
    void HandleSendDmx(lla::rpc::SimpleRpcController *controller,
                       lla::proto::Ack *reply);
    void HandleGetDmx(lla::rpc::SimpleRpcController *controller,
                      lla::proto::DmxData *reply);
    void HandleDeviceInfo(lla::rpc::SimpleRpcController *controller,
                          lla::proto::DeviceInfoReply *reply);
    void HandleUniverseInfo(lla::rpc::SimpleRpcController *controller,
                            lla::proto::UniverseInfoReply *reply);
    void HandleUniverseName(lla::rpc::SimpleRpcController *controller,
                            lla::proto::Ack *reply);
    void HandleUniverseMergeMode(lla::rpc::SimpleRpcController *controller,
                                 lla::proto::Ack *reply);
    void HandleRegister(lla::rpc::SimpleRpcController *controller,
                        lla::proto::Ack *reply);
    void HandlePatch(lla::rpc::SimpleRpcController *controller,
                     lla::proto::Ack *reply);
    void HandleDeviceConfig(lla::rpc::SimpleRpcController *controller,
                            lla::proto::DeviceConfigReply *reply);

  private:
    LlaClient(const LlaClient&);
    LlaClient operator=(const LlaClient&);
    static const unsigned int MAX_DMX = 512;

    lla::select_server::ConnectedSocket *m_socket;
    lla::LlaClientServiceImpl *m_client_service;
    lla::rpc::StreamRpcChannel *m_channel;
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
