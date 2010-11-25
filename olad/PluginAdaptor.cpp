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
 */

#include <string>
#include "olad/DeviceManager.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"

namespace ola {

using ola::network::SelectServer;

/*
 * Create a new PluginAdaptor
 * @param device_manager  pointer to a DeviceManager object
 * @param select_server pointer to the SelectServer object
 * @param preferences_factory pointer to the PreferencesFactory object
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
 * @return true on sucess, false on failure.
 */
bool PluginAdaptor::AddSocket(ola::network::Socket *socket) {
  return m_ss->AddSocket(socket);
}

/*
 * Register a connected socket with the select server.
 * @param socket the socket to register
 * @return true on sucess, false on failure.
 */
bool PluginAdaptor::AddSocket(ola::network::ConnectedSocket *socket,
                              bool delete_on_close) {
  return m_ss->AddSocket(socket, delete_on_close);
}


/*
 * Remove a socket from the select server
 */
bool PluginAdaptor::RemoveSocket(ola::network::Socket *socket) {
  return m_ss->RemoveSocket(socket);
}


/*
 * Remove a socket from the select server
 */
bool PluginAdaptor::RemoveSocket(ola::network::ConnectedSocket *socket) {
  return m_ss->RemoveSocket(socket);
}


/*
 * Register a repeating timeout
 * @param ms the time between function calls
 * @param closure the OlaClosure to call when the timeout expires
 * @return a timeout_id on success or K_INVALID_TIMEOUT on failure
 */
timeout_id PluginAdaptor::RegisterRepeatingTimeout(
    unsigned int ms,
    Closure<bool> *closure) {
  return m_ss->RegisterRepeatingTimeout(ms, closure);
}


/*
 * Register a single timeout
 * @param ms the time between function calls
 * @param closure the OlaClosure to call when the timeout expires
 * @return a timeout_id on success or K_INVALID_TIMEOUT on failure
 */
timeout_id PluginAdaptor::RegisterSingleTimeout(
    unsigned int ms,
    SingleUseClosure<void> *closure) {
  return m_ss->RegisterSingleTimeout(ms, closure);
}


/*
 * Remove a timeout
 * @param id the id of the timeout to remove
 */
void PluginAdaptor::RemoveTimeout(timeout_id id) {
  m_ss->RemoveTimeout(id);
}


/*
 * Register a device
 * @param dev  the device to register
 * @return true on success, false on error
 */
bool PluginAdaptor::RegisterDevice(AbstractDevice *device) const {
  return m_device_manager->RegisterDevice(device);
}


/*
 * Unregister a device
 * @param dev  the device to unregister
 * @return true on success, false on error
 */
bool PluginAdaptor::UnregisterDevice(AbstractDevice *device) const {
  return m_device_manager->UnregisterDevice(device);
}


/*
 * Create a new preferences container
 * @return a Preferences object
 */
Preferences *PluginAdaptor::NewPreference(const string &name) const {
  return m_preferences_factory->NewPreference(name);
}


/*
 * Return the wake up time for the select server
 * @return a TimeStamp object
 */
const TimeStamp *PluginAdaptor::WakeUpTime() const {
  return m_ss->WakeUpTime();
}
}  // ola
