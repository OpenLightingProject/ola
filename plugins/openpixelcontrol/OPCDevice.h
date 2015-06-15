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
 * OPCDevice.h
 * The Open Pixel Control Device.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_OPENPIXELCONTROL_OPCDEVICE_H_
#define PLUGINS_OPENPIXELCONTROL_OPCDEVICE_H_

#include <memory>
#include <string>

#include "ola/network/Socket.h"
#include "olad/Device.h"
#include "plugins/openpixelcontrol/OPCClient.h"
#include "plugins/openpixelcontrol/OPCServer.h"

namespace ola {

class AbstractPlugin;

namespace plugin {
namespace openpixelcontrol {

class OPCServerDevice: public ola::Device {
 public:
  /**
   * @brief Create a new OPC server device.
   * @param owner the Plugin that owns this device
   * @param plugin_adaptor the PluginAdaptor to use
   * @param preferences the Preferences container.
   * @param listen_addr the IP:port to listen on.
   */
  OPCServerDevice(AbstractPlugin *owner,
                  PluginAdaptor *plugin_adaptor,
                  Preferences *preferences,
                  const ola::network::IPV4SocketAddress listen_addr);

  std::string DeviceId() const;

  bool AllowMultiPortPatching() const { return true; }

 protected:
  bool StartHook();

 private:
  PluginAdaptor* const m_plugin_adaptor;
  Preferences* const m_preferences;
  const ola::network::IPV4SocketAddress m_listen_addr;
  std::auto_ptr<class OPCServer> m_server;

  DISALLOW_COPY_AND_ASSIGN(OPCServerDevice);
};

class OPCClientDevice: public ola::Device {
 public:
  /**
   * @brief Create a new OPC client device.
   * @param owner the Plugin that owns this device
   * @param plugin_adaptor the PluginAdaptor to use
   * @param preferences the Preferences container.
   * @param target the IP:port to connect to.
   */
  OPCClientDevice(AbstractPlugin *owner,
                  PluginAdaptor *plugin_adaptor,
                  Preferences *preferences,
                  const ola::network::IPV4SocketAddress target);

  std::string DeviceId() const;

  bool AllowMultiPortPatching() const { return true; }

 protected:
  bool StartHook();

 private:
  PluginAdaptor* const m_plugin_adaptor;
  Preferences* const m_preferences;
  const ola::network::IPV4SocketAddress m_target;
  std::auto_ptr<class OPCClient> m_client;

  DISALLOW_COPY_AND_ASSIGN(OPCClientDevice);
};
}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OPENPIXELCONTROL_OPCDEVICE_H_
