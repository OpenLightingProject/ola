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
 * PathportDevice.h
 * Interface for the pathport device
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_PATHPORT_PATHPORTDEVICE_H_
#define PLUGINS_PATHPORT_PATHPORTDEVICE_H_

#include <string>
#include "olad/Device.h"
#include "ola/io/SelectServer.h"
#include "plugins/pathport/PathportNode.h"

namespace ola {
namespace plugin {
namespace pathport {

class PathportDevice: public ola::Device {
 public:
    PathportDevice(class PathportPlugin *owner,
                   class Preferences *preferences,
                   class PluginAdaptor *plugin_adaptor);

    std::string DeviceId() const { return "1"; }
    PathportNode *GetNode() const { return m_node; }
    bool SendArpReply();

    static const char K_DEFAULT_NODE_NAME[];
    static const char K_DSCP_KEY[];
    static const char K_NODE_ID_KEY[];
    static const char K_NODE_IP_KEY[];
    static const char K_NODE_NAME_KEY[];

 protected:
    bool StartHook();
    void PrePortStop();
    void PostPortStop();

 private:
    class Preferences *m_preferences;
    class PluginAdaptor *m_plugin_adaptor;
    PathportNode *m_node;
    ola::thread::timeout_id m_timeout_id;

    static const char PATHPORT_DEVICE_NAME[];
    static const uint32_t PORTS_PER_DEVICE = 8;
    static const int ADVERTISEMENT_PERIOD_MS = 6000;
};
}  // namespace pathport
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_PATHPORT_PATHPORTDEVICE_H_
