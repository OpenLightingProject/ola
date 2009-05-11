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

namespace select_server {
  class Socket;
  class SocketManager;
  class SelectServer;
  class FDListener;
  class FDManager;
}

class LlaClosure;

using std::string;
using lla::select_server::Socket;
using lla::select_server::SocketManager;
using lla::select_server::SelectServer;
using lla::select_server::FDListener;
using lla::select_server::FDManager;

class PluginAdaptor {
  public :
    enum Direction{READ, WRITE};

    PluginAdaptor(class DeviceManager *device_manager,
                  SelectServer *select_server,
                  class PreferencesFactory *preferences_factory);
    int RegisterFD(int fd,
                   PluginAdaptor::Direction dir,
                   FDListener *listener,
                   FDManager *manager=NULL) const;
    int UnregisterFD(int fd, PluginAdaptor::Direction dir) const;
    int AddSocket(class Socket *socket,
                  class SocketManager *manager=NULL) const;
    int RemoveSocket(class Socket *socket) const;

    bool RegisterTimeout(int ms, LlaClosure *closure, bool repeat=true) const;
    int RegisterLoopCallback(FDListener *listener) const;

    int RegisterDevice(class AbstractDevice *device) const;
    int UnregisterDevice(class AbstractDevice *device) const;

    class Preferences *NewPreference(const string &name) const;

  private :
    PluginAdaptor(const PluginAdaptor&);
    PluginAdaptor& operator=(const PluginAdaptor&);

    DeviceManager *m_device_manager;
    class lla::select_server::SelectServer *m_ss;
    class PreferencesFactory *m_preferences_factory;

};

} //lla
#endif
