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
 * The provides operations on a lla_device.
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_PLUGINADAPTOR_H
#define LLA_PLUGINADAPTOR_H

#include <stdio.h>
#include <string>

namespace lla {

namespace network {
  class Socket;
  class SocketManager;
  class SelectServer;
}

class Closure;
class SingleUseClosure;

using std::string;
using lla::network::Socket;
using lla::network::SocketManager;
using lla::network::SelectServer;

class PluginAdaptor {
  public :
    PluginAdaptor(class DeviceManager *device_manager,
                  SelectServer *select_server,
                  class PreferencesFactory *preferences_factory);

    bool AddSocket(class Socket *socket) const;
    bool RemoveSocket(class Socket *socket) const;
    bool RegisterRepeatingTimeout(int ms, Closure *closure) const;
    bool RegisterSingleTimeout(int ms, SingleUseClosure *closure) const;

    bool RegisterDevice(class AbstractDevice *device) const;
    bool UnregisterDevice(class AbstractDevice *device) const;

    class Preferences *NewPreference(const string &name) const;

  private :
    PluginAdaptor(const PluginAdaptor&);
    PluginAdaptor& operator=(const PluginAdaptor&);

    DeviceManager *m_device_manager;
    class lla::network::SelectServer *m_ss;
    class PreferencesFactory *m_preferences_factory;
};

} //lla
#endif
