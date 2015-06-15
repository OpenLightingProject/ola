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
 * KiNetNode.h
 * Header file for the KiNetNode class.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_KINET_KINETNODE_H_
#define PLUGINS_KINET_KINETNODE_H_

#include <memory>

#include "ola/DmxBuffer.h"
#include "ola/io/BigEndianStream.h"
#include "ola/io/IOQueue.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Socket.h"

namespace ola {
namespace plugin {
namespace kinet {

class KiNetNode {
 public:
    KiNetNode(ola::io::SelectServerInterface *ss,
              ola::network::UDPSocketInterface *socket = NULL);
    virtual ~KiNetNode();

    bool Start();
    bool Stop();

    // The following apply to Input Ports (those which send data)
    bool SendDMX(const ola::network::IPV4Address &target,
                 const ola::DmxBuffer &buffer);

 private:
    bool m_running;
    ola::io::SelectServerInterface *m_ss;
    ola::io::IOQueue m_output_queue;
    ola::io::BigEndianOutputStream m_output_stream;
    ola::network::Interface m_interface;
    std::auto_ptr<ola::network::UDPSocketInterface> m_socket;

    KiNetNode(const KiNetNode&);
    KiNetNode& operator=(const KiNetNode&);

    void SocketReady();
    void PopulatePacketHeader(uint16_t msg_type);
    bool InitNetwork();

    static const uint16_t KINET_PORT = 6038;
    static const uint32_t KINET_MAGIC_NUMBER = 0x0401dc4a;
    static const uint16_t KINET_VERSION_ONE = 0x0100;
    static const uint16_t KINET_DMX_MSG = 0x0101;
};
}  // namespace kinet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_KINET_KINETNODE_H_
