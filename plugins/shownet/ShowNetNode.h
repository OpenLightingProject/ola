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
 * ShowNetNode.h
 * Header file for the ShowNetNode class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_SHOWNET_SHOWNETNODE_H_
#define PLUGINS_SHOWNET_SHOWNETNODE_H_

#include <string>
#include <map>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"
#include "ola/dmx/RunLengthEncoder.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/Socket.h"
#include "plugins/shownet/ShowNetPackets.h"

namespace ola {
namespace plugin {
namespace shownet {

class ShowNetNode {
 public:
    explicit ShowNetNode(const std::string &ip_address);
    virtual ~ShowNetNode();

    bool Start();
    bool Stop();
    void SetName(const std::string &name);

    bool SendDMX(unsigned int universe, const ola::DmxBuffer &buffer);
    bool SetHandler(unsigned int universe,
                    DmxBuffer *buffer,
                    ola::Callback0<void> *handler);
    bool RemoveHandler(unsigned int universe);

    const ola::network::Interface &GetInterface() const {
      return m_interface;
    }

    ola::network::UDPSocket* GetSocket() { return m_socket; }
    void SocketReady();

    static const uint16_t SHOWNET_MAX_UNIVERSES = 8;

    friend class ShowNetNodeTest;

 private:
    typedef struct {
      DmxBuffer *buffer;
      Callback0<void> *closure;
    } universe_handler;

    bool m_running;
    uint16_t m_packet_count;
    std::string m_node_name;
    std::string m_preferred_ip;
    std::map<unsigned int, universe_handler> m_handlers;
    ola::network::Interface m_interface;
    ola::dmx::RunLengthEncoder m_encoder;
    ola::network::UDPSocket *m_socket;

    bool HandlePacket(const shownet_packet *packet, unsigned int size);
    bool HandleCompressedPacket(const shownet_compressed_dmx *packet,
                                unsigned int packet_size);
    unsigned int BuildCompressedPacket(shownet_packet *packet,
                                       unsigned int universe,
                                       const DmxBuffer &buffer);
    bool InitNetwork();

    static const uint16_t SHOWNET_PORT = 2501;
    // In the shownet spec, the pass(2) and name(9) fields are combined with the
    // compressed data. This means the indices referenced in indexBlocks are
    // off by 11.
    static const uint16_t MAGIC_INDEX_OFFSET = 11;

    DISALLOW_COPY_AND_ASSIGN(ShowNetNode);
};
}  // namespace shownet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SHOWNET_SHOWNETNODE_H_
