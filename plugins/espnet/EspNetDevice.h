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
 * EspNetDevice.h
 * Interface for the espnet device
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef ESPNETDEVICE_H
#define ESPNETDEVICE_H

#include <llad/Device.h>
#include <llad/Plugin.h>
#include <llad/PluginAdaptor.h>
#include <llad/Preferences.h>
#include <lla/select_server/Socket.h>

#include <espnet/espnet.h>

#include "common.h"

namespace lla {
namespace plugin {

using std::string;
using lla::select_server::ConnectedSocket;
using lla::select_server::SocketListener;
using lla::Plugin;
using lla::Preferences;
using lla::PluginAdaptor;

class EspNetDevice: public lla::Device, public SocketListener {
  public:
    EspNetDevice(Plugin *owner,
                 const string &name,
                 Preferences *prefs,
                 const PluginAdaptor *plugin_adaptor);
    ~EspNetDevice();

    bool Start();
    bool Stop();
    espnet_node EspnetNode() const;
    int SocketReady(ConnectedSocket *socket);

  private:
    lla::Preferences *m_preferences;
    const lla::PluginAdaptor *m_plugin_adaptor;
    espnet_node m_node;
    ConnectedSocket *m_socket;
    bool m_enabled;
};

} //plugin
} //lla

#endif
