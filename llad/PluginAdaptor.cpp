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
 * PluginAdaptor.cpp
 * Provides a wrapper for the DeviceManager and SelectServer objects so that
 * the plugins can register devices and file handles for events
 * Copyright (C) 2005-2008 Simon Newton
 *
 *
 */

#include <llad/PluginAdaptor.h>
#include <llad/Preferences.h>
#include <lla/network/SelectServer.h>

#include "DeviceManager.h"

namespace lla {

using lla::network::SelectServer;

/*
 * Create a new pluginadaptor
 *
 * @param  m_device_manager  pointer to a devicemanager object
 * @param  m_ss  pointer to the m_sswork object
 */
PluginAdaptor::PluginAdaptor(DeviceManager *device_manager,
                             SelectServer *select_server,
                             PreferencesFactory *preferences_factory):
  m_device_manager(device_manager),
  m_ss(select_server),
  m_preferences_factory(preferences_factory) {
}


/*
 * Register a socket with the select server.
 * @param socket the socket to register
 * @param closure the closure to call when this socket is ready, ownership of
 *   the closure is transferred to the select server.
 * @param manager the manager to call when the socket is closed
 * @return true on sucess, false on failure.
 */
bool PluginAdaptor::AddSocket(class Socket *socket,
                              Closure *on_data,
                              SingleUseClosure *on_close) const {
  return m_ss->AddSocket(socket, on_data, on_close);
}


/*
 * Remove a socket from the select server
 */
bool PluginAdaptor::RemoveSocket(class Socket *socket) const {
  return m_ss->RemoveSocket(socket);
}


/*
 * register a repeating timeout
 * @param ms the time between function calls
 * @param closure the LlaClosure to call when the timeout expires
 * @return true on success, false on failure
 */
bool PluginAdaptor::RegisterRepeatingTimeout(int ms, Closure *closure) const {
  return m_ss->RegisterRepeatingTimeout(ms, closure);
}


/*
 * register a single timeout
 * @param ms the time between function calls
 * @param closure the LlaClosure to call when the timeout expires
 * @return true on success, false on failure
 */
bool PluginAdaptor::RegisterSingleTimeout(int ms,
                                          SingleUseClosure *closure) const {
  return m_ss->RegisterSingleTimeout(ms, closure);
}


/*
 * Register a device
 * @param dev  the device to register
 * @return 0 on success, non 0 on error
 */
int PluginAdaptor::RegisterDevice(AbstractDevice *device) const {
  return m_device_manager->RegisterDevice(device);
}


/*
 * Unregister a device
 *
 * @param dev  the device to unregister
 * @return 0 on success, non 0 on error
 */
int PluginAdaptor::UnregisterDevice(AbstractDevice *device) const {
  return m_device_manager->UnregisterDevice(device);
}


/*
 * Create a new preferences container
 *
 * @return a Preferences object
 */
Preferences *PluginAdaptor::NewPreference(const string &name) const {
  return m_preferences_factory->NewPreference(name);
}


} //lla
