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
 * SigNetDevice.h
 * The SigNet Device.
 * Copyright (C) 2026 Peter Newman
 */

#ifndef PLUGINS_SIGNET_SIGNETDEVICE_H_
#define PLUGINS_SIGNET_SIGNETDEVICE_H_

#include <memory>
#include <string>
#include <vector>
#include "ola/io/SelectServerInterface.h"
#include "olad/Device.h"
#include "plugins/signet/SigNetNode.h"

namespace ola {

class AbstractPlugin;

namespace plugin {
namespace signet {

class SigNetInputPort;
class SigNetOutputPort;

class SigNetDevice: public Device {
 public:
  struct SigNetDeviceOptions {
   public:
    SigNetDeviceOptions()
      : input_ports(0),
        output_ports(0) {
    }
    unsigned int input_ports;
    unsigned int output_ports;
  };

  SigNetDevice(AbstractPlugin *owner,
               PluginAdaptor *plugin_adaptor,
               const SigNetDeviceOptions &options);

  std::string DeviceId() const { return "1"; }

 protected:
  PluginAdaptor *m_plugin_adaptor;
  std::auto_ptr<class SigNetNode> m_signet_node;

  bool StartHook();

  static const char DEVICE_NAME[];

 private:
  const SigNetDeviceOptions m_options;
  std::vector<SigNetInputPort*> m_input_ports;
  std::vector<SigNetOutputPort*> m_output_ports;

  SigNetInputPort *GetSigNetInputPort(unsigned int port_id);
  SigNetOutputPort *GetSigNetOutputPort(unsigned int port_id);
};
}  // namespace signet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SIGNET_SIGNETDEVICE_H_
