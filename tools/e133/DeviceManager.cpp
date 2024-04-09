/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * DeviceManager.cpp
 * Copyright (C) 2013 Simon Newton
 */

#include <memory>
#include <string>
#include <vector>

#include "ola/e133/DeviceManager.h"
#include "ola/io/SelectServer.h"
#include "ola/network/IPV4Address.h"
#include "tools/e133/DeviceManagerImpl.h"

namespace ola {
namespace e133 {

using ola::NewCallback;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using std::string;


/**
 * Construct a new DeviceManager
 * @param ss a pointer to a SelectServerInterface to use
 * @param cid the CID of this controller.
 */
DeviceManager::DeviceManager(ola::io::SelectServerInterface *ss,
                             ola::e133::MessageBuilder *message_builder)
    : m_impl(new DeviceManagerImpl(ss, message_builder)) {
}


/**
 * Clean up
 */
DeviceManager::~DeviceManager() {}


/**
 * Set the callback to be run when RDMNet data is received from a device.
 * @param callback the RDMMessageCallback to run when data is received.
 */
void DeviceManager::SetRDMMessageCallback(RDMMessageCallback *callback) {
  m_impl->SetRDMMessageCallback(callback);
}


/**
 * Set the callback to be run when we become the designated controller for a
 * device.
 */
void DeviceManager::SetAcquireDeviceCallback(AcquireDeviceCallback *callback) {
  m_impl->SetAcquireDeviceCallback(callback);
}


/*
 * Set the callback to be run when we lose the designated controller status for
 * a device.
 */
void DeviceManager::SetReleaseDeviceCallback(ReleaseDeviceCallback *callback) {
  m_impl->SetReleaseDeviceCallback(callback);
}


/**
 * Start maintaining a connection to this device.
 */
void DeviceManager::AddDevice(const IPV4Address &ip_address) {
  m_impl->AddDevice(ip_address);
}


/**
 * Remove a device, closing the connection if we have one.
 */
void DeviceManager::RemoveDevice(const IPV4Address &ip_address) {
  m_impl->RemoveDevice(ip_address);
}


/**
 * Remove a device if there is no open connection.
 */
void DeviceManager::RemoveDeviceIfNotConnected(const IPV4Address &ip_address) {
  m_impl->RemoveDeviceIfNotConnected(ip_address);
}


/**
 * Populate the vector with the devices that we are the designated controller
 * for.
 */
void DeviceManager::ListManagedDevices(vector<IPV4Address> *devices) const {
  m_impl->ListManagedDevices(devices);
}
}  // namespace e133
}  // namespace ola
