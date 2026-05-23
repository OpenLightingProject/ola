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
 * OlaServerServiceImpl.h
 * Implementation of the OlaService interface
 * Copyright (C) 2005 Simon Newton
 */

#include <memory>
#include <string>
#include <vector>
#include "common/protocol/Ola.pb.h"
#include "common/protocol/OlaService.pb.h"
#include "ola/Callback.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"

#ifndef OLAD_OLASERVERSERVICEIMPL_H_
#define OLAD_OLASERVERSERVICEIMPL_H_

namespace ola {

class Universe;

/**
 * @brief The OLA Server RPC methods.
 *
 * After the RPC system un-marshalls the data, it invokes the methods of this
 * class. This therefore contains all the methods a client can invoke on the
 * server.
 *
 * There is no client specific member data, so a single OlaServerServiceImpl
 * is created. Any OLA client data is passed via the user data in the
 * ola::rpc::RpcSession object, accessible via the ola::rpc::RpcController.
 */
class OlaServerServiceImpl : public ola::proto::OlaServerService {
 public:
  /**
   * @brief A Callback used to reload all the plugins.
   */
  typedef Callback0<void> ReloadPluginsCallback;

  /**
   * @brief Create a new OlaServerServiceImpl.
   */
  OlaServerServiceImpl(class UniverseStore *universe_store,
                       class DeviceManager *device_manager,
                       class PluginManager *plugin_manager,
                       class PortManager *port_manager,
                       class ClientBroker *broker,
                       const class TimeStamp *wake_up_time,
                       ReloadPluginsCallback *reload_plugins_callback);

  ~OlaServerServiceImpl() {}

  /**
   * @brief Returns the current DMX values for a particular universe.
   */
  void GetDmx(ola::rpc::RpcController* controller,
              const ola::proto::UniverseRequest* request,
              ola::proto::DmxData* response,
              ola::rpc::RpcService::CompletionCallback* done);


  /**
   * @brief Register a client to receive DMX data.
   */
  void RegisterForDmx(ola::rpc::RpcController* controller,
                      const ola::proto::RegisterDmxRequest* request,
                      ola::proto::Ack* response,
                      ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Update the DMX values for a single universe.
   */
  void UpdateDmxData(ola::rpc::RpcController* controller,
                     const ola::proto::DmxData* request,
                     ola::proto::Ack* response,
                     ola::rpc::RpcService::CompletionCallback* done);
  /**
   * @brief Handle a streaming DMX update, no response is sent.
   */
  void StreamDmxData(ola::rpc::RpcController* controller,
                     const ::ola::proto::DmxData* request,
                     ::ola::proto::STREAMING_NO_RESPONSE* response,
                     ola::rpc::RpcService::CompletionCallback* done);


  /**
   * @brief Sets the name of a universe.
   */
  void SetUniverseName(ola::rpc::RpcController* controller,
                       const ola::proto::UniverseNameRequest* request,
                       ola::proto::Ack* response,
                       ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Set the merge mode for a universe.
   */
  void SetMergeMode(ola::rpc::RpcController* controller,
                    const ola::proto::MergeModeRequest* request,
                    ola::proto::Ack* response,
                    ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Patch a port to a universe.
   */
  void PatchPort(ola::rpc::RpcController* controller,
                 const ola::proto::PatchPortRequest* request,
                 ola::proto::Ack* response,
                 ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Set the priority of one or more ports.
   */
  void SetPortPriority(ola::rpc::RpcController* controller,
                       const ola::proto::PortPriorityRequest* request,
                       ola::proto::Ack* response,
                       ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Returns information on the active universes.
   */
  void GetUniverseInfo(ola::rpc::RpcController* controller,
                       const ola::proto::OptionalUniverseRequest* request,
                       ola::proto::UniverseInfoReply* response,
                       ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Return info on available plugins.
   */
  void GetPlugins(ola::rpc::RpcController* controller,
                  const ola::proto::PluginListRequest* request,
                  ola::proto::PluginListReply* response,
                  ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Reload the plugins.
   */
  void ReloadPlugins(ola::rpc::RpcController* controller,
                     const ola::proto::PluginReloadRequest* request,
                     ola::proto::Ack* response,
                     ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Return the description for a plugin.
   */
  void GetPluginDescription(
      ola::rpc::RpcController* controller,
      const ola::proto::PluginDescriptionRequest* request,
      ola::proto::PluginDescriptionReply* response,
      ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Return the state for a plugin.
   */
  void GetPluginState(
      ola::rpc::RpcController* controller,
      const ola::proto::PluginStateRequest* request,
      ola::proto::PluginStateReply* response,
      ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Change the state of plugins.
   */
  void SetPluginState(
      ola::rpc::RpcController* controller,
      const ola::proto::PluginStateChangeRequest* request,
      ola::proto::Ack* response,
      ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Return information on available devices.
   */
  void GetDeviceInfo(ola::rpc::RpcController* controller,
                     const ola::proto::DeviceInfoRequest* request,
                     ola::proto::DeviceInfoReply* response,
                     ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Handle a GetCandidatePorts request.
   */
  void GetCandidatePorts(ola::rpc::RpcController* controller,
                         const ola::proto::OptionalUniverseRequest* request,
                         ola::proto::DeviceInfoReply* response,
                         ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Handle a ConfigureDevice request.
   */
  void ConfigureDevice(ola::rpc::RpcController* controller,
                       const ola::proto::DeviceConfigRequest* request,
                       ola::proto::DeviceConfigReply* response,
                       ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Fetch the UID list for a universe.
   */
  void GetUIDs(ola::rpc::RpcController* controller,
               const ola::proto::UniverseRequest* request,
               ola::proto::UIDListReply* response,
               ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Force RDM discovery for a universe.
   */
  void ForceDiscovery(ola::rpc::RpcController* controller,
                      const ola::proto::DiscoveryRequest* request,
                      ola::proto::UIDListReply* response,
                      ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Handle an RDM Command.
   */
  void RDMCommand(ola::rpc::RpcController* controller,
                  const ::ola::proto::RDMRequest* request,
                  ola::proto::RDMResponse* response,
                  ola::rpc::RpcService::CompletionCallback* done);


  /**
   * @brief Handle an RDM Discovery Command.
   *
   * This is used by the RDM responder tests. Normally clients don't need to
   * send raw discovery packets and can just use the GetUIDs method.
   */
  void RDMDiscoveryCommand(ola::rpc::RpcController* controller,
                           const ::ola::proto::RDMDiscoveryRequest* request,
                           ola::proto::RDMResponse* response,
                           ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Set this client's source UID.
   */
  void SetSourceUID(ola::rpc::RpcController* controller,
                    const ::ola::proto::UID* request,
                    ola::proto::Ack* response,
                    ola::rpc::RpcService::CompletionCallback* done);

  /**
   * @brief Send Timecode
   */
  void SendTimeCode(ola::rpc::RpcController* controller,
                    const ::ola::proto::TimeCode* request,
                    ::ola::proto::Ack* response,
                    ola::rpc::RpcService::CompletionCallback* done);

 private:
  void HandleRDMResponse(ola::proto::RDMResponse* response,
                         ola::rpc::RpcService::CompletionCallback* done,
                         bool include_raw_packets,
                         ola::rdm::RDMReply *reply);
  void RDMDiscoveryComplete(unsigned int universe,
                            ola::rpc::RpcService::CompletionCallback* done,
                            ola::proto::UIDListReply *response,
                            const ola::rdm::UIDSet &uids);

  void MissingUniverseError(ola::rpc::RpcController* controller);
  void MissingPluginError(ola::rpc::RpcController* controller);
  void MissingDeviceError(ola::rpc::RpcController* controller);
  void MissingPortError(ola::rpc::RpcController* controller);

  void AddPlugin(class AbstractPlugin *plugin,
                 ola::proto::PluginInfo *plugin_info) const;
  void AddDevice(class AbstractDevice *device,
                 unsigned int alias,
                 ola::proto::DeviceInfoReply* response) const;
  void AddUniverse(const Universe *universe,
                   ola::proto::UniverseInfoReply *universe_info_reply) const;

  template <class PortClass>
  void PopulatePort(const PortClass &port,
                    ola::proto::PortInfo *port_info) const;

  void SetProtoUID(const ola::rdm::UID &uid, ola::proto::UID *pb_uid);

  class Client* GetClient(ola::rpc::RpcController *controller);

  UniverseStore *m_universe_store;
  DeviceManager *m_device_manager;
  class PluginManager *m_plugin_manager;
  class PortManager *m_port_manager;
  class ClientBroker *m_broker;
  const class TimeStamp *m_wake_up_time;
  std::unique_ptr<ReloadPluginsCallback> m_reload_plugins_callback;
};
}  // namespace ola
#endif  // OLAD_OLASERVERSERVICEIMPL_H_
