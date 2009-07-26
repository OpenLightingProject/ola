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

#include "common/protocol/Ola.pb.h"

#ifndef OLA_SERVER_SERVICE_IMPL_H
#define OLA_SERVER_SERVICE_IMPL_H

namespace ola {

using namespace ola::proto;
using google::protobuf::RpcController;

class OlaServerServiceImpl: public ola::proto::OlaServerService {
  public:
    OlaServerServiceImpl(class UniverseStore *universe_store,
                         class DeviceManager *device_manager,
                         class PluginLoader *plugin_loader,
                         class Client *client,
                         class ExportMap *export_map):
      m_universe_store(universe_store),
      m_device_manager(device_manager),
      m_plugin_loader(plugin_loader),
      m_client(client),
      m_export_map(export_map) {}
    ~OlaServerServiceImpl() {}

    void GetDmx(RpcController* controller,
                const DmxReadRequest* request,
                DmxData* response,
                google::protobuf::Closure* done);
    void RegisterForDmx(RpcController* controller,
                        const RegisterDmxRequest* request,
                        Ack* response,
                        google::protobuf::Closure* done);
    void UpdateDmxData(RpcController* controller,
                       const DmxData* request,
                       Ack* response,
                       google::protobuf::Closure* done);
    void SetUniverseName(RpcController* controller,
                         const UniverseNameRequest* request,
                         Ack* response,
                         google::protobuf::Closure* done);
    void SetMergeMode(RpcController* controller,
                      const MergeModeRequest* request,
                      Ack* response,
                      google::protobuf::Closure* done);
    void PatchPort(RpcController* controller,
                   const PatchPortRequest* request,
                   Ack* response,
                   google::protobuf::Closure* done);
    void GetUniverseInfo(RpcController* controller,
                         const UniverseInfoRequest* request,
                         UniverseInfoReply* response,
                         google::protobuf::Closure* done);
    void GetPluginInfo(RpcController* controller,
                       const PluginInfoRequest* request,
                       PluginInfoReply* response,
                       google::protobuf::Closure* done);
    void GetDeviceInfo(RpcController* controller,
                       const DeviceInfoRequest* request,
                       DeviceInfoReply* response,
                       google::protobuf::Closure* done);
    void ConfigureDevice(RpcController* controller,
                         const DeviceConfigRequest* request,
                         DeviceConfigReply* response,
                         google::protobuf::Closure* done);

    Client *GetClient() const { return m_client; }

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
                   PluginInfoReply* response,
                   bool include_description) const;
    void AddDevice(class AbstractDevice *device,
                   unsigned int alias,
                   DeviceInfoReply* response) const;

    UniverseStore *m_universe_store;
    DeviceManager *m_device_manager;
    PluginLoader *m_plugin_loader;
    Client *m_client;
    ExportMap *m_export_map;
};


class OlaServerServiceImplFactory {
  public:
    OlaServerServiceImpl *New(UniverseStore *universe_store,
                              DeviceManager *device_manager,
                              class PluginLoader *plugin_loader,
                              class Client *client,
                              class ExportMap *export_map);
};

} // ola

#endif
