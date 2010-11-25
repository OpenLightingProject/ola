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

#ifndef INCLUDE_OLAD_PLUGINADAPTOR_H_
#define INCLUDE_OLAD_PLUGINADAPTOR_H_

#include <stdio.h>
#include <string>
#include <ola/Clock.h>  // NOLINT
#include <ola/Closure.h>  // NOLINT
#include <ola/network/SelectServer.h>  // NOLINT
#include <ola/network/SelectServerInterface.h>  // NOLINT

namespace ola {

using ola::network::timeout_id;

class PluginAdaptor: public ola::network::SelectServerInterface {
  public :
    PluginAdaptor(class DeviceManager *device_manager,
                  ola::network::SelectServer *select_server,
                  class PreferencesFactory *preferences_factory);

    // The following methods are part of the SelectServerInterface
    bool AddSocket(ola::network::Socket *socket);
    bool AddSocket(ola::network::ConnectedSocket *socket,
                   bool delete_on_close = false);
    bool RemoveSocket(ola::network::Socket *socket);
    bool RemoveSocket(ola::network::ConnectedSocket *socket);
    timeout_id RegisterRepeatingTimeout(unsigned int ms,
                                        Closure<bool> *closure);
    timeout_id RegisterSingleTimeout(unsigned int ms,
                                     SingleUseClosure<void> *closure);
    void RemoveTimeout(timeout_id id);

    const TimeStamp *WakeUpTime() const;

    // These are the extra bits for the plugins
    bool RegisterDevice(class AbstractDevice *device) const;
    bool UnregisterDevice(class AbstractDevice *device) const;
    class Preferences *NewPreference(const std::string &name) const;

  private:
    PluginAdaptor(const PluginAdaptor&);
    PluginAdaptor& operator=(const PluginAdaptor&);

    DeviceManager *m_device_manager;
    ola::network::SelectServer *m_ss;
    class PreferencesFactory *m_preferences_factory;
};
}  // ola
#endif  // INCLUDE_OLAD_PLUGINADAPTOR_H_
