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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DeviceManager.cpp
 * Copyright (C) 2013 Simon Newton
 * The DeviceManager maintains a TCP connection to each E1.33 device.
 */

#ifndef TOOLS_E133_DEVICEMANAGER_H_
#define TOOLS_E133_DEVICEMANAGER_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ola/BaseTypes.h>
#include <ola/Callback.h>
#include <ola/Clock.h>
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

#include "plugins/e131/e131/RDMInflator.h"
#include "plugins/e131/e131/E133Inflator.h"
#include "plugins/e131/e131/RootInflator.h"
#include "plugins/e131/e131/TCPTransport.h"

using ola::TimeInterval;
using ola::network::TCPSocket;
using ola::network::IPV4Address;

using std::auto_ptr;
using std::string;
using std::vector;

/**
 * This class is responsible for maintaining connections to E1.33 devices.
 * TODO(simon): Some of this code can be re-used for the controller side. See
 * if we can factor it out.
 */
class DeviceManager {
  public:
    /*
     * The callback used to receive RDMNet layer messages from the devices.
     * @returns true if the data should be acknowledged, false otherwise.
     */
    typedef ola::Callback3<bool,
                           const ola::plugin::e131::TransportHeader&,
                           const ola::plugin::e131::E133Header&,
                           const string&> RDMMesssageCallback;

    // Run when we acquire designated controller status for a device.
    typedef ola::Callback1<void, const IPV4Address&> AcquireDeviceCallback;

    // Run when we give up (or lose) designated controller status.
    typedef ola::Callback1<void, const IPV4Address&> ReleaseDeviceCallback;

    DeviceManager(ola::io::SelectServerInterface *ss,
                  ola::e133::MessageBuilder *message_builder);
    ~DeviceManager();

    // Ownership of the callbacks is transferred.
    void SetRDMMessageCallback(RDMMesssageCallback *callback);
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
    auto_ptr<RDMMesssageCallback> m_rdm_callback;
    auto_ptr<AcquireDeviceCallback> m_acquire_device_cb_;
    auto_ptr<ReleaseDeviceCallback> m_release_device_cb_;

    ola::io::SelectServerInterface *m_ss;

    ola::network::TCPSocketFactory m_tcp_socket_factory;
    ola::network::AdvancedTCPConnector m_connector;
    ola::LinearBackoffPolicy m_backoff_policy;

    ola::e133::MessageBuilder *m_message_builder;

    // inflators
    ola::plugin::e131::RootInflator m_root_inflator;
    ola::plugin::e131::E133Inflator m_e133_inflator;
    ola::plugin::e131::RDMInflator m_rdm_inflator;

    /*
     * Avoid passing pointers to the DeviceState objects in callbacks.
     * When we start removing stale entries this is going to break!
     * Maybe this won't be a problem since we'll never delete the entry for a
     * a device we have a connection to. Think about this.
     */
    void OnTCPConnect(TCPSocket *socket);
    void ReceiveTCPData(IPV4Address ip_address,
                        ola::plugin::e131::IncomingTCPTransport *transport);
    void SocketUnhealthy(IPV4Address address);
    void SocketClosed(IPV4Address address);
    void RLPDataReceived(const ola::plugin::e131::TransportHeader &header);

    void EndpointRequest(
        const ola::plugin::e131::TransportHeader &transport_header,
        const ola::plugin::e131::E133Header &e133_header,
        const string &raw_request);

    static const TimeInterval TCP_CONNECT_TIMEOUT;
    static const TimeInterval INITIAL_TCP_RETRY_DELAY;
    static const TimeInterval MAX_TCP_RETRY_DELAY;
};
#endif  // TOOLS_E133_DEVICEMANAGER_H_
