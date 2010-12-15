/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * OlaServerServiceImpl.h
 * Implemtation of the OlaService interface
 * Copyright (C) 2005 - 2008 Simon Newton
 */

#include <vector>
#include <string>
#include "common/protocol/Ola.pb.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/RDMCommand.h"
#include "olad/ClientBroker.h"

#ifndef OLAD_OLASERVERSERVICEIMPL_H_
#define OLAD_OLASERVERSERVICEIMPL_H_

namespace ola {

using google::protobuf::RpcController;
using ola::proto::Ack;

/*
 * This class forms part of the core of the Server. It implements all the
 * methods the clients can invoke on the server.o
 *
 * This class doesn't contain any client specific data, which means we only
 * need to create one of them. Any client specific data is passed in
 * per-method.
 */
class OlaServerServiceImpl {
  public:
    OlaServerServiceImpl(class UniverseStore *universe_store,
                         class DeviceManager *device_manager,
                         class PluginManager *plugin_manager,
                         class ExportMap *export_map,
                         class PortManager *port_manager,
                         class ClientBroker *broker,
                         const class TimeStamp *wake_up_time,
                         const ola::rdm::UID &uid):
      m_universe_store(universe_store),
      m_device_manager(device_manager),
      m_plugin_manager(plugin_manager),
      m_export_map(export_map),
      m_port_manager(port_manager),
      m_broker(broker),
      m_wake_up_time(wake_up_time),
      m_uid(uid) {}
    ~OlaServerServiceImpl();

    void GetDmx(RpcController* controller,
                const ola::proto::UniverseRequest* request,
                ola::proto::DmxData* response,
                google::protobuf::Closure* done);
    void RegisterForDmx(RpcController* controller,
                        const ola::proto::RegisterDmxRequest* request,
                        Ack* response,
                        google::protobuf::Closure* done,
                        class Client *client);
    void UpdateDmxData(RpcController* controller,
                       const ola::proto::DmxData* request,
                       Ack* response,
                       google::protobuf::Closure* done,
                       class Client *client);
    void StreamDmxData(RpcController* controller,
                       const ::ola::proto::DmxData* request,
                       ::ola::proto::STREAMING_NO_RESPONSE* response,
                       ::google::protobuf::Closure* done,
                       class Client *client);
    void SetUniverseName(RpcController* controller,
                         const ola::proto::UniverseNameRequest* request,
                         Ack* response,
                         google::protobuf::Closure* done);
    void SetMergeMode(RpcController* controller,
                      const ola::proto::MergeModeRequest* request,
                      Ack* response,
                      google::protobuf::Closure* done);
    void PatchPort(RpcController* controller,
                   const ola::proto::PatchPortRequest* request,
                   Ack* response,
                   google::protobuf::Closure* done);
    void SetPortPriority(RpcController* controller,
                         const ola::proto::PortPriorityRequest* request,
                         Ack* response,
                         google::protobuf::Closure* done);
    void GetUniverseInfo(RpcController* controller,
                         const ola::proto::OptionalUniverseRequest* request,
                         ola::proto::UniverseInfoReply* response,
                         google::protobuf::Closure* done);
    void GetPlugins(RpcController* controller,
                       const ola::proto::PluginListRequest* request,
                       ola::proto::PluginListReply* response,
                       google::protobuf::Closure* done);
    void GetPluginDescription(
        RpcController* controller,
        const ola::proto::PluginDescriptionRequest* request,
        ola::proto::PluginDescriptionReply* response,
        google::protobuf::Closure* done);
    void GetDeviceInfo(RpcController* controller,
                       const ola::proto::DeviceInfoRequest* request,
                       ola::proto::DeviceInfoReply* response,
                       google::protobuf::Closure* done);
    void GetCandidatePorts(RpcController* controller,
                           const ola::proto::OptionalUniverseRequest* request,
                           ola::proto::DeviceInfoReply* response,
                           google::protobuf::Closure* done);
    void ConfigureDevice(RpcController* controller,
                         const ola::proto::DeviceConfigRequest* request,
                         ola::proto::DeviceConfigReply* response,
                         google::protobuf::Closure* done);
    void GetUIDs(RpcController* controller,
                 const ola::proto::UniverseRequest* request,
                 ola::proto::UIDListReply* response,
                 google::protobuf::Closure* done);
    void ForceDiscovery(RpcController* controller,
                        const ola::proto::UniverseRequest* request,
                        ola::proto::Ack* response,
                        google::protobuf::Closure* done);
    void RDMCommand(RpcController* controller,
                    const ::ola::proto::RDMRequest* request,
                    ola::proto::RDMResponse* response,
                    google::protobuf::Closure* done,
                    const UID *uid,
                    class Client *client);
    void SetSourceUID(RpcController* controller,
                      const ::ola::proto::UID* request,
                      ola::proto::Ack* response,
                      google::protobuf::Closure* done);

    void HandleRDMResponse(RpcController* controller,
                           ola::proto::RDMResponse* response,
                           google::protobuf::Closure* done,
                           ola::rdm::rdm_response_status status,
                           const ola::rdm::RDMResponse *rdm_response,
                           const std::vector<std::string> &packets);

  private:
    void MissingUniverseError(RpcController* controller,
                              google::protobuf::Closure* done);
    void MissingPluginError(RpcController* controller,
                            google::protobuf::Closure* done);
    void MissingDeviceError(RpcController* controller,
                            google::protobuf::Closure* done);
    void MissingPortError(RpcController* controller,
                          google::protobuf::Closure* done);

    void AddPlugin(class AbstractPlugin *plugin,
                   ola::proto::PluginListReply* response) const;
    void AddDevice(class AbstractDevice *device,
                   unsigned int alias,
                   ola::proto::DeviceInfoReply* response) const;

    template <class PortClass>
    void PopulatePort(const PortClass &port,
                      ola::proto::PortInfo *port_info) const;

    UniverseStore *m_universe_store;
    DeviceManager *m_device_manager;
    class PluginManager *m_plugin_manager;
    class ExportMap *m_export_map;
    class PortManager *m_port_manager;
    class ClientBroker *m_broker;
    const class TimeStamp *m_wake_up_time;
    ola::rdm::UID m_uid;
};


/*
 * This implements the client specific portion of the OlaServerService.
 * In other words, it implements the methods that the clients can invoke on the
 * server and holds the client data so the requests can be performed. It pretty
 * much passes everything through to the OlaServerServiceImpl.
 */
class OlaClientService: public ola::proto::OlaServerService {
  public:
    OlaClientService(class Client *client,
                     OlaServerServiceImpl *impl):
      m_client(client),
      m_impl(impl),
      m_uid(NULL) {
    }
    ~OlaClientService();

    void GetDmx(RpcController* controller,
                const ola::proto::UniverseRequest* request,
                ola::proto::DmxData* response,
                google::protobuf::Closure* done) {
      m_impl->GetDmx(controller, request, response, done);
    }

    void RegisterForDmx(RpcController* controller,
                        const ola::proto::RegisterDmxRequest* request,
                        Ack* response,
                        google::protobuf::Closure* done) {
      m_impl->RegisterForDmx(controller, request, response, done, m_client);
    }

    void UpdateDmxData(RpcController* controller,
                       const ola::proto::DmxData* request,
                       Ack* response,
                       google::protobuf::Closure* done) {
      m_impl->UpdateDmxData(controller, request, response, done, m_client);
    }

    void StreamDmxData(RpcController* controller,
                       const ::ola::proto::DmxData* request,
                       ::ola::proto::STREAMING_NO_RESPONSE* response,
                       ::google::protobuf::Closure* done) {
      m_impl->StreamDmxData(controller, request, response, done, m_client);
    }

    void SetUniverseName(RpcController* controller,
                         const ola::proto::UniverseNameRequest* request,
                         Ack* response,
                         google::protobuf::Closure* done) {
      m_impl->SetUniverseName(controller, request, response, done);
    }

    void SetMergeMode(RpcController* controller,
                      const ola::proto::MergeModeRequest* request,
                      Ack* response,
                      google::protobuf::Closure* done) {
      m_impl->SetMergeMode(controller, request, response, done);
    }

    void PatchPort(RpcController* controller,
                   const ola::proto::PatchPortRequest* request,
                   Ack* response,
                   google::protobuf::Closure* done) {
      m_impl->PatchPort(controller, request, response, done);
    }

    void SetPortPriority(RpcController* controller,
                         const ola::proto::PortPriorityRequest* request,
                         Ack* response,
                         google::protobuf::Closure* done) {
      m_impl->SetPortPriority(controller, request, response, done);
    }

    void GetUniverseInfo(RpcController* controller,
                         const ola::proto::OptionalUniverseRequest* request,
                         ola::proto::UniverseInfoReply* response,
                         google::protobuf::Closure* done) {
      m_impl->GetUniverseInfo(controller, request, response, done);
    }

    void GetPlugins(RpcController* controller,
                       const ola::proto::PluginListRequest* request,
                       ola::proto::PluginListReply* response,
                       google::protobuf::Closure* done) {
      m_impl->GetPlugins(controller, request, response, done);
    }

    void GetPluginDescription(
        RpcController* controller,
        const ola::proto::PluginDescriptionRequest* request,
        ola::proto::PluginDescriptionReply* response,
        google::protobuf::Closure* done) {
      m_impl->GetPluginDescription(controller, request, response, done);
    }

    void GetDeviceInfo(RpcController* controller,
                       const ola::proto::DeviceInfoRequest* request,
                       ola::proto::DeviceInfoReply* response,
                       google::protobuf::Closure* done) {
      m_impl->GetDeviceInfo(controller, request, response, done);
    }

    void GetCandidatePorts(RpcController* controller,
                           const ola::proto::OptionalUniverseRequest* request,
                           ola::proto::DeviceInfoReply* response,
                           google::protobuf::Closure* done) {
      m_impl->GetCandidatePorts(controller, request, response, done);
    }

    void ConfigureDevice(RpcController* controller,
                         const ola::proto::DeviceConfigRequest* request,
                         ola::proto::DeviceConfigReply* response,
                         google::protobuf::Closure* done) {
      m_impl->ConfigureDevice(controller, request, response, done);
    }

    void GetUIDs(RpcController* controller,
                 const ola::proto::UniverseRequest* request,
                 ola::proto::UIDListReply* response,
                 google::protobuf::Closure* done) {
      m_impl->GetUIDs(controller, request, response, done);
    }

    void ForceDiscovery(RpcController* controller,
                        const ola::proto::UniverseRequest* request,
                        ola::proto::Ack* response,
                        google::protobuf::Closure* done) {
      m_impl->ForceDiscovery(controller, request, response, done);
    }

    void RDMCommand(RpcController* controller,
                    const ::ola::proto::RDMRequest* request,
                    ola::proto::RDMResponse* response,
                    google::protobuf::Closure* done) {
      m_impl->RDMCommand(controller, request, response, done, m_uid, m_client);
    }

    void SetSourceUID(RpcController* controller,
                      const ::ola::proto::UID* request,
                      ola::proto::Ack* response,
                      google::protobuf::Closure* done);

    Client *GetClient() const { return m_client; }

  private:
    class Client *m_client;
    OlaServerServiceImpl *m_impl;
    ola::rdm::UID *m_uid;
};


/**
 * This is the factory method for creating new OlaServerService(s)
 */
class OlaClientServiceFactory {
  public:
    OlaClientService *New(Client *client,
                          OlaServerServiceImpl *impl);
};
}  // ola
#endif  // OLAD_OLASERVERSERVICEIMPL_H_
