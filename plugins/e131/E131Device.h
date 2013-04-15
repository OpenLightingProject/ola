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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * E131Device.h
 * Interface for the E1.31 device
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef PLUGINS_E131_E131DEVICE_H_
#define PLUGINS_E131_E131DEVICE_H_

#include <string>
#include "ola/acn/CID.h"
#include "olad/Device.h"
#include "olad/Plugin.h"
#include "plugins/e131/messages/E131ConfigMessages.pb.h"

namespace ola {
namespace plugin {
namespace e131 {

using google::protobuf::RpcController;
using ola::Plugin;
using ola::plugin::e131::Request;

class E131Device: public ola::Device {
  public:
    struct E131DeviceOptions {
      unsigned int input_ports;
      unsigned int output_ports;
      bool use_rev2;
      bool prepend_hostname;
      bool ignore_preview;
      uint8_t dscp;

      E131DeviceOptions()
          : input_ports(0),
            output_ports(0),
            use_rev2(false),
            prepend_hostname(true),
            ignore_preview(true),
            dscp(0) {
      }
    };

    E131Device(Plugin *owner,
               const ola::acn::CID &cid,
               std::string ip_addr,
               class PluginAdaptor *plugin_adaptor,
               const E131DeviceOptions &options);

    string DeviceId() const { return "1"; }

    void Configure(RpcController *controller,
                   const string &request,
                   string *response,
                   google::protobuf::Closure *done);
  protected:
    bool StartHook();
    void PrePortStop();
    void PostPortStop();

  private:
    class PluginAdaptor *m_plugin_adaptor;
    class E131Node *m_node;
    bool m_use_rev2;
    bool m_prepend_hostname;
    bool m_ignore_preview;
    uint8_t m_dscp;
    const unsigned int m_input_port_count, m_output_port_count;
    std::string m_ip_addr;
    ola::acn::CID m_cid;

    void HandlePreviewMode(Request *request, string *response);
    void HandlePortStatusRequest(string *response);

    static const char DEVICE_NAME[];
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131DEVICE_H_
