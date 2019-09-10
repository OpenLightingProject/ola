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
 * Interface for the Art-Net device
 * Copyright (C) 2005 Simon Newton
 */

/**
 * @namespace ola::plugin::artnet
 * An Art-Net device is an instance of libartnet bound to a single IP address
 * Art-Net is limited to four ports per direction per IP, so in this case
 * our device has 8 ports :
 *
 * IDs 0-3 : Input ports (recv DMX)
 * IDs 4-7 : Output ports (send DMX)
 */

#ifndef PLUGINS_ARTNET_ARTNETDEVICE_H_
#define PLUGINS_ARTNET_ARTNETDEVICE_H_

#include <string>

#include "olad/Device.h"
#include "plugins/artnet/messages/ArtNetConfigMessages.pb.h"
#include "plugins/artnet/ArtNetNode.h"

namespace ola {

class Preferences;

namespace plugin {
namespace artnet {

class ArtNetDevice: public Device {
 public:
  /**
   * Create a new Art-Net Device
   */
  ArtNetDevice(AbstractPlugin *owner,
               class Preferences *preferences,
               class PluginAdaptor *plugin_adaptor);

  // only one Art-Net device
  std::string DeviceId() const { return "1"; }

  void EnterConfigurationMode() { m_node->EnterConfigurationMode(); }
  void ExitConfigurationMode() { m_node->ExitConfigurationMode(); }

  /**
   * Handle device config messages
   * @param controller An RpcController
   * @param request the request data
   * @param response the response to return
   * @param done the closure to call once the request is complete
   */
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
  static const unsigned int K_ARTNET_NET;
  static const unsigned int K_ARTNET_SUBNET;
  static const unsigned int K_DEFAULT_OUTPUT_PORT_COUNT;
  // 10s between polls when we're sending data, DMX-workshop uses 8s;
  static const unsigned int POLL_INTERVAL = 10000;

 protected:
  /**
   * Start this device
   * @return true on success, false on failure
   */
  bool StartHook();

  /**
   * Stop this device. This is called before the ports are deleted
   */
  void PrePortStop();

  /**
   * Stop this device
   */
  void PostPortStop();

 private:
  class Preferences *m_preferences;
  ArtNetNode *m_node;
  class PluginAdaptor *m_plugin_adaptor;
  ola::thread::timeout_id m_timeout_id;

  /**
   * Handle an options request
   */
  void HandleOptions(Request *request, std::string *response);

  /**
   * Handle a node list request
   */
  void HandleNodeList(Request *request,
                      std::string *response,
                      ola::rpc::RpcController *controller);
};
}  // namespace artnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_ARTNET_ARTNETDEVICE_H_
