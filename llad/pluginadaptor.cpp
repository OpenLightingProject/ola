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
#include <lla/select_server/FdListener.h>
#include <lla/select_server/TimeoutListener.h>
#include <lla/select_server/SelectServer.h>

#include "DeviceManager.h"

namespace lla {

using lla::select_server::SelectServer;
using lla::select_server::FDListener;
using lla::select_server::TimeoutListener;

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
 * register a fd
 *
 * @param fd    the file descriptor to register
 * @param dir    the direction we want
 * @param listener  the object to be notifies when the descriptor is ready
 * @param manager  the object to be notified if the listener returns an error
 *
 * @return 0 on success, non 0 on error
 */
int PluginAdaptor::RegisterFD(int fd, PluginAdaptor::Direction direction,
                              FDListener *listener,
                              FDManager *manager) const {
  SelectServer::Direction dir = (direction == PluginAdaptor::READ ?
                                 SelectServer::READ : SelectServer::WRITE);
  return m_ss->RegisterFD(fd, dir, listener, manager);
}


/*
 * Unregister a fd
 *
 * @param fd  the file descriptor to unregister
 * @param dir  the direction we'll interested in
 *
 * @return 0 on success, non 0 on error
 */
int PluginAdaptor::UnregisterFD(int fd,
                                PluginAdaptor::Direction direction) const {
  SelectServer::Direction dir = (direction == PluginAdaptor::READ ?
                                 SelectServer::READ : SelectServer::WRITE);
  return m_ss->UnregisterFD(fd, dir);
}


int PluginAdaptor::AddSocket(Socket *socket,
                             SocketManager *manager) const {
  return m_ss->AddSocket(socket, manager);
}


int PluginAdaptor::RemoveSocket(Socket *socket) const {
  return m_ss->RemoveSocket(socket);
}


/*
 * register a timeout
 *
 * @param second  the time between function calls
 *
 * @return the timeout id on success, 0 on error
 */
int PluginAdaptor::RegisterTimeout(int ms, TimeoutListener *listener) const {
  return m_ss->RegisterTimeout(ms, listener);
}


/*
 * register a loop function
 */
int PluginAdaptor::RegisterLoopCallback(FDListener *listener) const {
  return m_ss->RegisterLoopCallback(listener);
}


/*
 * Register a device
 *
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
