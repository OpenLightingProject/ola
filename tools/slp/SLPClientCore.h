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
 * SLPClientCore.h
 * The internal SLP Client class.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef TOOLS_SLP_SLPCLIENTCORE_H_
#define TOOLS_SLP_SLPCLIENTCORE_H_

#include <google/protobuf/stubs/common.h>
#include <string>
#include <vector>

#include "SLP.pb.h"
#include "SLPClient.h"
#include "common/rpc/SimpleRpcController.h"
#include "common/rpc/StreamRpcChannel.h"
#include "ola/Callback.h"
#include "ola/network/Socket.h"

namespace ola {
namespace slp {

class SLPClientCoreServiceImpl;

using std::string;
using ola::io::ConnectedDescriptor;
using ola::rpc::SimpleRpcController;
using ola::rpc::StreamRpcChannel;

class SLPClientCore {
  public:
    explicit SLPClientCore(ConnectedDescriptor *descriptor);
    ~SLPClientCore();

    bool Setup();
    bool Stop();

    /**
     * Register a service
     */
    bool RegisterService(
        const string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const string&, uint16_t> *callback);

    /**
     * Register a service that persists beyond the lifetime of this client.
     */
    bool RegisterPersistentService(
        const string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const string&, uint16_t> *callback);

    /**
     * DeRegister a service
     */
    bool DeRegisterService(
        const string &service,
        SingleUseCallback2<void, const string&, uint16_t> *callback);

    /**
     * Find a service
     */
    bool FindService(
        const vector<string> &scopes,
        const string &service,
        SingleUseCallback2<void, const string&,
                           const vector<SLPService> &> *callback);


    // unfortunately all of these need to be public because they're used in the
    // closures. That's why this class is wrapped in OlaClient or
    // OlaCallbackClient.
    typedef struct {
      SimpleRpcController *controller;
      ola::slp::proto::ServiceAck *reply;
      SingleUseCallback2<void, const string&, uint16_t> *callback;
    } register_arg;

    void HandleRegistration(register_arg *args);

    typedef struct {
      SimpleRpcController *controller;
      ola::slp::proto::ServiceReply *reply;
      SingleUseCallback2<void, const string&,
                         const vector<SLPService> &> *callback;
    } find_arg;

    void HandleFindRequest(find_arg *args);

  private:
    ConnectedDescriptor *m_descriptor;
    StreamRpcChannel *m_channel;
    ola::slp::proto::SLPService_Stub *m_stub;
    int m_connected;

    SLPClientCore(const SLPClientCore&);
    SLPClientCore operator=(const SLPClientCore&);

    bool GenericRegisterService(
        const string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const string&, uint16_t> *callback,
        bool persistant);

    template <typename arg_type, typename reply_type, typename callback_type>
    arg_type *NewArgs(
        SimpleRpcController *controller,
        reply_type reply,
        callback_type callback);

    template <typename arg_type>
    void FreeArgs(arg_type *args);
};


/**
 * Create a new args structure
 */
template <typename arg_type, typename reply_type, typename callback_type>
arg_type *SLPClientCore::NewArgs(
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
void SLPClientCore::FreeArgs(arg_type *args) {
  delete args->controller;
  delete args->reply;
  delete args;
}
}  // slp
}  // ola
#endif  // TOOLS_SLP_SLPCLIENTCORE_H_
