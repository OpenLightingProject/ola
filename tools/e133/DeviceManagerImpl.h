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
 * DeviceManagerImpl.h
 * Copyright (C) 2013 Simon Newton
 * The DeviceManagerImpl maintains a TCP connection to each E1.33 device.
 */

#ifndef TOOLS_E133_DEVICEMANAGERIMPL_H_
#define TOOLS_E133_DEVICEMANAGERIMPL_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <ola/Callback.h>
#include <ola/Clock.h>
#include <ola/Constants.h>
#include <ola/e133/MessageBuilder.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/AdvancedTCPConnector.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Socket.h>
#include <ola/network/TCPSocketFactory.h>

#include <memory>
#include <string>
#include <vector>

#include HASH_MAP_H

#include "libs/acn/RDMInflator.h"
#include "libs/acn/E133Inflator.h"
#include "libs/acn/RootInflator.h"
#include "libs/acn/TCPTransport.h"

namespace ola {
namespace e133 {

using ola::TimeInterval;
using ola::network::TCPSocket;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;

using std::auto_ptr;
using std::string;
using std::vector;

/**
 * This class is responsible for maintaining connections to E1.33 devices.
 * TODO(simon): Some of this code can be re-used for the controller side. See
 * if we can factor it out.
 */
class DeviceManagerImpl {
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

    DeviceManagerImpl(ola::io::SelectServerInterface *ss,
                  ola::e133::MessageBuilder *message_builder);
    ~DeviceManagerImpl();

    // Ownership of the callbacks is transferred.
    void SetRDMMessageCallback(RDMMessageCallback *callback);
    void SetAcquireDeviceCallback(AcquireDeviceCallback *callback);
    void SetReleaseDeviceCallback(ReleaseDeviceCallback *callback);

    void AddDevice(const IPV4Address &ip_address);
    void RemoveDevice(const IPV4Address &ip_address);
    void RemoveDeviceIfNotConnected(const IPV4Address &ip_address);
    void ListManagedDevices(vector<IPV4Address> *devices) const;

 private:
    // hash_map of IPs to DeviceState
    typedef HASH_NAMESPACE::HASH_MAP_CLASS<uint32_t, class DeviceState*>
      DeviceMap;

    DeviceMap m_device_map;
    auto_ptr<RDMMessageCallback> m_rdm_callback;
    auto_ptr<AcquireDeviceCallback> m_acquire_device_cb_;
    auto_ptr<ReleaseDeviceCallback> m_release_device_cb_;

    ola::io::SelectServerInterface *m_ss;

    ola::network::TCPSocketFactory m_tcp_socket_factory;
    ola::network::AdvancedTCPConnector m_connector;
    ola::LinearBackoffPolicy m_backoff_policy;

    ola::e133::MessageBuilder *m_message_builder;

    // inflators
    ola::acn::RootInflator m_root_inflator;
    ola::acn::E133Inflator m_e133_inflator;
    ola::acn::RDMInflator m_rdm_inflator;

    /*
     * Avoid passing pointers to the DeviceState objects in callbacks.
     * When we start removing stale entries this is going to break!
     * Maybe this won't be a problem since we'll never delete the entry for a
     * a device we have a connection to. Think about this.
     */
    void OnTCPConnect(TCPSocket *socket);
    void ReceiveTCPData(IPV4Address ip_address,
                        ola::acn::IncomingTCPTransport *transport);
    void SocketUnhealthy(IPV4Address address);
    void SocketClosed(IPV4Address address);
    void RLPDataReceived(const ola::acn::TransportHeader &header);

    void EndpointRequest(
        const ola::acn::TransportHeader *transport_header,
        const ola::acn::E133Header *e133_header,
        const string &raw_request);

    static const TimeInterval TCP_CONNECT_TIMEOUT;
    static const TimeInterval INITIAL_TCP_RETRY_DELAY;
    static const TimeInterval MAX_TCP_RETRY_DELAY;
};
}  // namespace e133
}  // namespace ola
#endif  // TOOLS_E133_DEVICEMANAGERIMPL_H_
