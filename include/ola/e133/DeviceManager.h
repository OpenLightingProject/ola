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
 * DeviceManager.h
 * Copyright (C) 2013 Simon Newton
 * The DeviceManager attempts to maintain a TCP connection to each E1.33 device.
 */

#ifndef INCLUDE_OLA_E133_DEVICEMANAGER_H_
#define INCLUDE_OLA_E133_DEVICEMANAGER_H_

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/e133/MessageBuilder.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>

#include <memory>
#include <string>
#include <vector>

namespace ola {
namespace e133 {

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using std::auto_ptr;
using std::string;
using std::vector;

/**
 * This class is responsible for maintaining connections to E1.33 devices. It
 * simply passes calls through to the DeviceManagerImpl so we don't have to
 * pull in all the headers.
 */
class DeviceManager {
 public:
    /*
     * The callback used to receive RDMNet layer messages from the devices.
     * @returns true if the data should be acknowledged, false otherwise.
     */
    typedef ola::Callback3<bool, const IPV4Address&, uint16_t,
                           const string&> RDMMessageCallback;

    // Run when we acquire designated controller status for a device.
    typedef ola::Callback1<void, const IPV4Address&> AcquireDeviceCallback;

    // Run when we give up (or lose) designated controller status.
    typedef ola::Callback1<void, const IPV4Address&> ReleaseDeviceCallback;

    DeviceManager(ola::io::SelectServerInterface *ss,
                  ola::e133::MessageBuilder *message_builder);
    ~DeviceManager();

    // Ownership of the callbacks is transferred.
    void SetRDMMessageCallback(RDMMessageCallback *callback);
    void SetAcquireDeviceCallback(AcquireDeviceCallback *callback);
    void SetReleaseDeviceCallback(ReleaseDeviceCallback *callback);

    void AddDevice(const IPV4Address &ip_address);
    void RemoveDevice(const IPV4Address &ip_address);
    void RemoveDeviceIfNotConnected(const IPV4Address &ip_address);
    void ListManagedDevices(vector<IPV4Address> *devices) const;

 private:
    class DeviceManagerImpl *m_impl;

    DISALLOW_COPY_AND_ASSIGN(DeviceManager);
};
}  // namespace e133
}  // namespace ola
#endif  // INCLUDE_OLA_E133_DEVICEMANAGER_H_
