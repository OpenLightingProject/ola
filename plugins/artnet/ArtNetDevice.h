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
 * ArtNetDevice.h
 * Interface for the ArtNet device
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_ARTNET_ARTNETDEVICE_H_
#define PLUGINS_ARTNET_ARTNETDEVICE_H_

#include <string>

#include "olad/Device.h"
#include "plugins/artnet/messages/ArtnetConfigMessages.pb.h"
#include "plugins/artnet/ArtNetNode.h"

namespace ola {

class Preferences;

namespace plugin {
namespace artnet {

class ArtNetDevice: public Device {
 public:
  ArtNetDevice(AbstractPlugin *owner,
               class Preferences *preferences,
               class PluginAdaptor *plugin_adaptor);

  // only one ArtNet device
  std::string DeviceId() const { return "1"; }

  void EnterConfigurationMode() { m_node->EnterConfigurationMode(); }
  void ExitConfigurationMode() { m_node->ExitConfigurationMode(); }

  void Configure(ola::rpc::RpcController *controller,
                 const std::string &request,
                 std::string *response,
                 ConfigureCallback *done);

  static const char K_ALWAYS_BROADCAST_KEY[];
  static const char K_DEVICE_NAME[];
  static const char K_IP_KEY[];
  static const char K_LIMITED_BROADCAST_KEY[];
  static const char K_LONG_NAME_KEY[];
  static const char K_LOOPBACK_KEY[];
  static const char K_NET_KEY[];
  static const char K_OUTPUT_PORT_KEY[];
  static const char K_SHORT_NAME_KEY[];
  static const char K_SUBNET_KEY[];
  // 10s between polls when we're sending data, DMX-workshop uses 8s;
  static const unsigned int POLL_INTERVAL = 10000;

 protected:
  bool StartHook();
  void PrePortStop();
  void PostPortStop();

 private:
  class Preferences *m_preferences;
  ArtNetNode *m_node;
  class PluginAdaptor *m_plugin_adaptor;
  ola::thread::timeout_id m_timeout_id;

  void HandleOptions(Request *request, std::string *response);
  void HandleNodeList(Request *request,
                      std::string *response,
                      ola::rpc::RpcController *controller);
};
}  // namespace artnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_ARTNET_ARTNETDEVICE_H_
