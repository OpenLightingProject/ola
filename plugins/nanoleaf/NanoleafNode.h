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
 * NanoleafNode.h
 * Header file for the NanoleafNode class.
 * Copyright (C) 2017 Peter Newman
 */

#ifndef PLUGINS_NANOLEAF_NANOLEAFNODE_H_
#define PLUGINS_NANOLEAF_NANOLEAFNODE_H_

#include <memory>
#include <vector>

#include "ola/DmxBuffer.h"
#include "ola/io/OutputStream.h"
#include "ola/io/IOQueue.h"
#include "ola/io/SelectServerInterface.h"
#include "ola/network/Interface.h"
#include "ola/network/SocketAddress.h"
#include "ola/network/Socket.h"

namespace ola {
namespace plugin {
namespace nanoleaf {

class NanoleafNode {
 public:
    NanoleafNode(ola::io::SelectServerInterface *ss,
                 std::vector<uint8_t> panels,
                 ola::network::UDPSocketInterface *socket = NULL);
    virtual ~NanoleafNode();

    bool Start();
    bool Stop();

    // The following apply to Input Ports (those which send data)
    bool SendDMX(const ola::network::IPV4SocketAddress &target,
                 const ola::DmxBuffer &buffer);

 private:
    bool m_running;
    ola::io::SelectServerInterface *m_ss;
    std::vector<uint8_t> m_panels;
    ola::io::IOQueue m_output_queue;
    ola::io::OutputStream m_output_stream;
    ola::network::Interface m_interface;
    std::auto_ptr<ola::network::UDPSocketInterface> m_socket;

    NanoleafNode(const NanoleafNode&);
    NanoleafNode& operator=(const NanoleafNode&);

    void SocketReady();
    bool InitNetwork();

    static const uint8_t NANOLEAF_FRAME_COUNT = 0x01;
    static const uint8_t NANOLEAF_WHITE_LEVEL = 0x00;
    static const uint8_t NANOLEAF_TRANSITION_TIME = 0x01;
    static const uint8_t NANOLEAF_SLOTS_PER_PANEL = 3;
};
}  // namespace nanoleaf
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_NANOLEAF_NANOLEAFNODE_H_
