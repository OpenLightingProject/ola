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
 * PathportDevice.h
 * Interface for the pathport device
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef PATHPORTDEVICE_H
#define PATHPORTDEVICE_H

#include <map>

#include <llad/Device.h>
#include <lla/network/Socket.h>

#include <pathport/pathport.h>

#include "PathportCommon.h"

namespace lla {
namespace plugin {

using lla::Plugin;
using lla::PluginAdaptor;
using lla::Preferences;
using lla::network::ConnectedSocket;
using lla::network::SocketListener;
using std::string;

class PathportDevice: public lla::Device, public SocketListener {
  public:
    PathportDevice(Plugin *owner,
                   const string &name,
                   class Preferences *prefs,
                   const PluginAdaptor *plugin_adaptor);
    ~PathportDevice();

    bool Start();
    bool Stop();
    pathport_node PathportNode() const;
    int SocketReady(ConnectedSocket *socket);

    int port_map(class Universe *uni, class PathportPort *prt);
    class PathportPort *GetPort_from_uni(int uni);

  private:
    class Preferences *m_preferences;
    const PluginAdaptor *m_plugin_adaptor;
    pathport_node m_node;
    bool m_enabled;
    map<int, class PathportPort*> m_portmap;
};

} //plugin
} //lla

#endif
