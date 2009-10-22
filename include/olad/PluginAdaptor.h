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
 * PluginAdaptor.h
 * The provides operations on a ola_device.
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef OLA_PLUGINADAPTOR_H
#define OLA_PLUGINADAPTOR_H

#include <stdio.h>
#include <string>
#include <ola/network/SelectServer.h>

namespace ola {

namespace network {
  class Socket;
  class ConnectedSocket;
  class SocketManager;
}

class Closure;
class SingleUseClosure;

using std::string;
using ola::network::Socket;
using ola::network::ConnectedSocket;
using ola::network::SocketManager;
using ola::network::SelectServer;
using ola::network::timeout_id;

class PluginAdaptor {
  public :
    PluginAdaptor(class DeviceManager *device_manager,
                  SelectServer *select_server,
                  class PreferencesFactory *preferences_factory);

    bool AddSocket(class Socket *socket) const;
    bool AddSocket(class ConnectedSocket *socket,
                   bool delete_on_close=false) const;
    bool RemoveSocket(class Socket *socket) const;
    bool RemoveSocket(class ConnectedSocket *socket) const;
    timeout_id RegisterRepeatingTimeout(int ms, Closure *closure) const;
    timeout_id RegisterSingleTimeout(int ms, SingleUseClosure *closure) const;
    void RemoveTimeout(timeout_id id) const;

    bool RegisterDevice(class AbstractDevice *device) const;
    bool UnregisterDevice(class AbstractDevice *device) const;

    class Preferences *NewPreference(const string &name) const;

  private:
    PluginAdaptor(const PluginAdaptor&);
    PluginAdaptor& operator=(const PluginAdaptor&);

    DeviceManager *m_device_manager;
    class ola::network::SelectServer *m_ss;
    class PreferencesFactory *m_preferences_factory;
};

} //ola
#endif
