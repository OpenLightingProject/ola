/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ArtNetDevice.h
 * Interface for the ArtNet device
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef PLUGINS_ARTNET_ARTNETDEVICE_H_
#define PLUGINS_ARTNET_ARTNETDEVICE_H_

#include <artnet/artnet.h>
#include <string>

#include "ola/network/Socket.h"
#include "olad/Device.h"
#include "olad/PluginAdaptor.h"
#include "plugins/artnet/messages/ArtnetConfigMessages.pb.h"

namespace ola {


class Preferences;

namespace plugin {
namespace artnet {

using google::protobuf::RpcController;
using ola::Device;
using ola::plugin::artnet::Request;
using std::string;

class ArtNetDevice: public Device {
  public:
    ArtNetDevice(AbstractPlugin *owner,
                 const string &name,
                 class Preferences *preferences,
                 bool debug,
                 const TimeStamp *wake_time);
    ~ArtNetDevice();

    bool Start();
    bool Stop();
    bool AllowLooping() const { return false; }
    bool AllowMultiPortPatching() const { return false; }

    // only one ArtNet device
    string DeviceId() const { return "1"; }
    int SocketReady();
    ola::network::UnmanagedSocket *GetSocket() { return m_socket; }

    void Configure(RpcController *controller,
                   const string &request,
                   string *response,
                   google::protobuf::Closure *done);

    static const char K_SHORT_NAME_KEY[];
    static const char K_LONG_NAME_KEY[];
    static const char K_SUBNET_KEY[];
    static const char K_IP_KEY[];

  private:
    class Preferences *m_preferences;
    ola::network::UnmanagedSocket *m_socket;
    artnet_node m_node;
    string m_short_name;
    string m_long_name;
    uint8_t m_subnet;
    bool m_enabled;
    bool m_debug;
    const TimeStamp *m_wake_time;

    void HandleOptions(Request *request, string *response);
};
}  // arntnet
}  // plugin
}  // ola
#endif  // PLUGINS_ARTNET_ARTNETDEVICE_H_
