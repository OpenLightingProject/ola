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
 * SandNetDevice.h
 * Interface for the sandnet device
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_SANDNET_SANDNETDEVICE_H_
#define PLUGINS_SANDNET_SANDNETDEVICE_H_

#include <string>
#include "olad/Device.h"
#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "plugins/sandnet/SandNetCommon.h"
#include "plugins/sandnet/SandNetNode.h"

namespace ola {
namespace plugin {
namespace sandnet {

class SandNetDevice: public ola::Device {
 public:
    SandNetDevice(class SandNetPlugin *owner,
                  class Preferences *prefs,
                  class PluginAdaptor *plugin_adaptor);

    std::string DeviceId() const { return "1"; }
    SandNetNode *GetNode() { return m_node; }

    bool SendAdvertisement();

    static const char IP_KEY[];
    static const char NAME_KEY[];

 protected:
    bool StartHook();
    void PrePortStop();
    void PostPortStop();

 private:
    class Preferences *m_preferences;
    class PluginAdaptor *m_plugin_adaptor;
    SandNetNode *m_node;
    ola::thread::timeout_id m_timeout_id;

    static const char SANDNET_DEVICE_NAME[];
    // the number of input ports to create
    static const unsigned int INPUT_PORTS = 8;
    // send an advertisement every 2s.
    static const int ADVERTISEMENT_PERIOD_MS = 2000;
};
}  // namespace sandnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SANDNET_SANDNETDEVICE_H_
