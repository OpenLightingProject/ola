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
 * SandNetNode.h
 * Header file for the SandNetNode class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_SANDNET_SANDNETNODE_H_
#define PLUGINS_SANDNET_SANDNETNODE_H_

#include <map>
#include <string>
#include <vector>
#include <utility>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Interface.h"
#include "ola/network/Socket.h"
#include "ola/dmx/RunLengthEncoder.h"
#include "plugins/sandnet/SandNetPackets.h"

namespace ola {
namespace plugin {
namespace sandnet {


class SandNetNode {
 public:
    typedef enum {
      SANDNET_PORT_MODE_DISABLED,
      SANDNET_PORT_MODE_OUT,
      SANDNET_PORT_MODE_IN,
      SANDNET_PORT_MODE_MOUT,
      SANDNET_PORT_MODE_MIN
    } sandnet_port_type;

    // stupid Windows, 'interface' seems to be a struct so we use iface here.
    explicit SandNetNode(const ola::network::Interface &iface);
    ~SandNetNode();

    const ola::network::Interface &GetInterface() const {
      return m_interface;
    }

    void SetName(const std::string &name) {
      m_node_name = name;
    }
    bool Start();
    bool Stop();
    std::vector<ola::network::UDPSocket*> GetSockets();
    void SocketReady(ola::network::UDPSocket *socket);

    bool SetHandler(uint8_t group,
                    uint8_t universe,
                    DmxBuffer *buffer,
                    Callback0<void> *closure);
    bool RemoveHandler(uint8_t group, uint8_t universe);

    bool SetPortParameters(uint8_t port_id, sandnet_port_type type,
                           uint8_t group, uint8_t universe);
    bool SendAdvertisement();
    bool SendDMX(uint8_t port_id, const DmxBuffer &buffer);

 private:
    typedef struct {
      uint8_t group;
      uint8_t universe;
      sandnet_port_type type;
    } sandnet_port;

    typedef struct {
      DmxBuffer *buffer;
      Callback0<void> *closure;
    } universe_handler;

    typedef std::pair<uint8_t, uint8_t> group_universe_pair;
    typedef std::map<group_universe_pair, universe_handler> universe_handlers;

    bool InitNetwork();

    bool HandleCompressedDMX(const sandnet_compressed_dmx &dmx_packet,
                             unsigned int size);

    bool HandleDMX(const sandnet_dmx &dmx_packet,
                   unsigned int size);
    bool SendUncompressedDMX(uint8_t port_id, const DmxBuffer &buffer);
    bool SendPacket(const sandnet_packet &packet,
                    unsigned int size,
                    bool is_control = false);

    bool m_running;
    std::string m_node_name;

    sandnet_port m_ports[SANDNET_MAX_PORTS];
    universe_handlers m_handlers;
    ola::network::Interface m_interface;
    ola::network::UDPSocket m_control_socket;
    ola::network::UDPSocket m_data_socket;
    ola::dmx::RunLengthEncoder m_encoder;
    ola::network::IPV4SocketAddress m_control_addr;
    ola::network::IPV4SocketAddress m_data_addr;

    static const uint16_t CONTROL_PORT;
    static const uint16_t DATA_PORT;
    static const char CONTROL_ADDRESS[];
    static const char DATA_ADDRESS[];
    static const char DEFAULT_NODE_NAME[];
    static const uint32_t FIRMWARE_VERSION = 0x00050501;
};
}  // namespace sandnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SANDNET_SANDNETNODE_H_
