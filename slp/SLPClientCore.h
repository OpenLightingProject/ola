/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * SLPClientCore.h
 * The internal SLP Client class.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef SLP_SLPCLIENTCORE_H_
#define SLP_SLPCLIENTCORE_H_

#include <google/protobuf/stubs/common.h>
#include <string>
#include <vector>

#include "common/rpc/RpcController.h"
#include "common/rpc/RpcChannel.h"
#include "ola/Callback.h"
#include "ola/network/Socket.h"
#include "ola/slp/SLPClient.h"
#include "slp/SLP.pb.h"
#include "slp/SLPService.pb.h"

namespace ola {
namespace slp {

class SLPClientCoreServiceImpl;


class SLPClientCore {
 public:
    explicit SLPClientCore(ola::io::ConnectedDescriptor *descriptor);
    ~SLPClientCore();

    bool Setup();
    bool Stop();

    void SetCloseHandler(ola::SingleUseCallback0<void> *callback);

    /**
     * Register a service
     */
    bool RegisterService(
        const std::vector<std::string> &scopes,
        const std::string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const std::string&, uint16_t> *callback);

    /**
     * Register a service that persists beyond the lifetime of this client.
     */
    bool RegisterPersistentService(
        const std::vector<std::string> &scopes,
        const std::string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const std::string&, uint16_t> *callback);

    /**
     * DeRegister a service
     */
    bool DeRegisterService(
        const std::vector<std::string> &scopes,
        const std::string &service,
        SingleUseCallback2<void, const std::string&, uint16_t> *callback);

    /**
     * Find a service
     */
    bool FindService(
        const std::vector<std::string> &scopes,
        const std::string &service,
        SingleUseCallback2<void,
                           const std::string&,
                           const std::vector<URLEntry> &> *callback);

    /**
     * Get info about the server
     */
    bool GetServerInfo(
        SingleUseCallback2<void,
                           const std::string&,
                           const ServerInfo&> *callback);


    // unfortunately all of these need to be public because they're used in the
    // closures. That's why this class is wrapped in OlaClient or
    // OlaCallbackClient.
    typedef struct {
      ola::rpc::RpcController *controller;
      ola::slp::proto::ServiceAck *reply;
      SingleUseCallback2<void, const std::string&, uint16_t> *callback;
    } register_arg;

    void HandleRegistration(register_arg *args);

    typedef struct {
      ola::rpc::RpcController *controller;
      ola::slp::proto::ServiceReply *reply;
      SingleUseCallback2<void, const std::string&,
                         const std::vector<URLEntry> &> *callback;
    } find_arg;

    void HandleFindRequest(find_arg *args);

    typedef struct {
      ola::rpc::RpcController *controller;
      ola::slp::proto::ServerInfoReply *reply;
      SingleUseCallback2<void, const std::string&, const ServerInfo&> *callback;
    } server_info_arg;

    void HandleServerInfo(server_info_arg *args);

 private:
    ola::io::ConnectedDescriptor *m_descriptor;
    ola::rpc::RpcChannel *m_channel;
    ola::slp::proto::SLPService_Stub *m_stub;
    int m_connected;

    SLPClientCore(const SLPClientCore&);
    SLPClientCore operator=(const SLPClientCore&);

    bool GenericRegisterService(
        const std::vector<std::string> &scopes,
        const std::string &service,
        uint16_t lifetime,
        SingleUseCallback2<void, const std::string&, uint16_t> *callback,
        bool persistant);

    template <typename arg_type, typename reply_type, typename callback_type>
    arg_type *NewArgs(
        ola::rpc::RpcController *controller,
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
    ola::rpc::RpcController *controller,
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
}  // namespace slp
}  // namespace ola
#endif  // SLP_SLPCLIENTCORE_H_
