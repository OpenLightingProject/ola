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
                const std::vector<ola::network::IPV4Address> &power_supplies,
                class PluginAdaptor *plugin_adaptor);

    // Only one KiNet device
    std::string DeviceId() const { return "1"; }

    // We can stream the same universe to multiple IPs
    // TODO(Peter): Remove this when we have a device per IP
    bool AllowMultiPortPatching() const { return true; }

 protected:
    bool StartHook();
    void PrePortStop();
    void PostPortStop();

 private:
    const std::vector<ola::network::IPV4Address> m_power_supplies;
    class KiNetNode *m_node;
    class PluginAdaptor *m_plugin_adaptor;
};
}  // namespace kinet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_KINET_KINETDEVICE_H_
