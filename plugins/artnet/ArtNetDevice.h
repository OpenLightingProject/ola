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

#ifndef ARTNETDEVICE_H
#define ARTNETDEVICE_H

#include <lla/network/Socket.h>
#include <llad/Device.h>
#include <llad/PluginAdaptor.h>

#include <artnet/artnet.h>
#include "messages/ArtnetConfigMessages.pb.h"

namespace lla {

using google::protobuf::Closure;
using google::protobuf::RpcController;
using lla::plugin::artnet::Request;

class Preferences;

namespace plugin {

using lla::Device;
using lla::network::ConnectedSocket;
using std::string;

class ArtNetDevice : public Device, public lla::network::SocketListener {
  public:
    ArtNetDevice(AbstractPlugin *owner,
                 const string &name,
                 class Preferences *preferences,
                 bool debug);
    ~ArtNetDevice();

    bool Start();
    bool Stop();
    artnet_node GetArtnetNode() const;
    int SaveConfig() const;
    int SocketReady(ConnectedSocket *socket);
    ConnectedSocket *GetSocket() { return m_socket; }

    void Configure(RpcController *controller,
                   const string &request,
                   string *response,
                   Closure *done);

  private:
    class Preferences *m_preferences;
    ConnectedSocket *m_socket;
    artnet_node m_node;
    string m_short_name;
    string m_long_name;
    uint8_t m_subnet;
    bool m_enabled;
    bool m_debug;

    void HandleOptions(Request *request, string *response);
    static const string K_SHORT_NAME_KEY;
    static const string K_LONG_NAME_KEY;
    static const string K_SUBNET_KEY;
    static const string K_IP_KEY;
};

} //plugin
} //lla

#endif
