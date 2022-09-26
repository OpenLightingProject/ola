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
 * KiNetDevice.h
 * Interface for the KiNet device
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_KINET_KINETDEVICE_H_
#define PLUGINS_KINET_KINETDEVICE_H_

#include <string>
#include <vector>

#include "ola/network/IPV4Address.h"
#include "olad/Device.h"

namespace ola {
namespace plugin {
namespace kinet {

class KiNetDevice: public ola::Device {
 public:
  KiNetDevice(AbstractPlugin *owner,
              const ola::network::IPV4Address &power_supply,
              class PluginAdaptor *plugin_adaptor,
              class KiNetNode *node,
              class Preferences *preferences);

  std::string DeviceId() const;
  static std::string ModeKey(const ola::network::IPV4Address &power_supply);
  std::string ModeKey() const;
  void SetDefaults();

  // We can stream the same universe to multiple IPs
  // TODO(Peter): Remove this when we have a device per IP
  bool AllowMultiPortPatching() const { return true; }

  static const char DMXOUT_MODE[];
  static const char PORTOUT_MODE[];

 protected:
  const ola::network::IPV4Address m_power_supply;
  class PluginAdaptor *m_plugin_adaptor;
  class KiNetNode *m_node;
  class Preferences *m_preferences;

 private:
  static const char KINET_DEVICE_NAME[];
};

class KiNetPortOutDevice: public KiNetDevice {
 public:
  KiNetPortOutDevice(AbstractPlugin *owner,
                     const ola::network::IPV4Address &power_supply,
                     class PluginAdaptor *plugin_adaptor,
                     class KiNetNode *node,
                     class Preferences *preferences);

  std::string PortCountKey() const;
  void SetDefaults();

 protected:
  bool StartHook();

 private:
  static const char KINET_PORT_OUT_DEVICE_NAME[];
};

class KiNetDmxOutDevice: public KiNetDevice {
 public:
  KiNetDmxOutDevice(AbstractPlugin *owner,
                    const ola::network::IPV4Address &power_supply,
                    class PluginAdaptor *plugin_adaptor,
                    class KiNetNode *node,
                    class Preferences *preferences);

 protected:
  bool StartHook();

 private:
  static const char KINET_DMX_OUT_DEVICE_NAME[];
};
}  // namespace kinet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_KINET_KINETDEVICE_H_
