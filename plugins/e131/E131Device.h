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
 * E131Device.h
 * Interface for the E1.31 device
 * Copyright (C) 2007 Simon Newton
 */

#ifndef PLUGINS_E131_E131DEVICE_H_
#define PLUGINS_E131_E131DEVICE_H_

#include <memory>
#include <string>
#include <vector>
#include "libs/acn/E131Node.h"
#include "ola/acn/CID.h"
#include "olad/Device.h"
#include "olad/Plugin.h"
#include "plugins/e131/messages/E131ConfigMessages.pb.h"

namespace ola {
namespace plugin {
namespace e131 {

class E131InputPort;
class E131OutputPort;

class E131Device: public ola::Device {
 public:
  struct E131DeviceOptions : public ola::acn::E131Node::Options {
   public:
    E131DeviceOptions()
      : ola::acn::E131Node::Options(),
        input_ports(0),
        output_ports(0) {
    }
    unsigned int input_ports;
    unsigned int output_ports;
  };

  E131Device(ola::Plugin *owner,
             const ola::acn::CID &cid,
             std::string ip_addr,
             class PluginAdaptor *plugin_adaptor,
             const E131DeviceOptions &options);

  std::string DeviceId() const { return "1"; }

  void Configure(ola::rpc::RpcController *controller,
                 const std::string &request,
                 std::string *response,
                 ConfigureCallback *done);

 protected:
  bool StartHook();
  void PrePortStop();
  void PostPortStop();

 private:
  class PluginAdaptor *m_plugin_adaptor;
  std::auto_ptr<ola::acn::E131Node> m_node;
  const E131DeviceOptions m_options;
  std::vector<E131InputPort*> m_input_ports;
  std::vector<E131OutputPort*> m_output_ports;
  std::string m_ip_addr;
  ola::acn::CID m_cid;

  void HandlePreviewMode(const ola::plugin::e131::Request *request,
                         std::string *response);
  void HandlePortStatusRequest(std::string *response);
  void HandleSourceListRequest(const ola::plugin::e131::Request *request,
                               std::string *response);

  E131InputPort *GetE131InputPort(unsigned int port_id);
  E131OutputPort *GetE131OutputPort(unsigned int port_id);

  static const char DEVICE_NAME[];
};
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131DEVICE_H_
