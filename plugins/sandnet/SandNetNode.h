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
 * SandNetNode.h
 * Header file for the SandNetNode class
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef PLUGINS_SANDNET_SANDNETNODE_H_
#define PLUGINS_SANDNET_SANDNETNODE_H_

#include <map>
#include <string>
#include <vector>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/Socket.h"
#include "ola/RunLengthEncoder.h"
#include "plugins/sandnet/SandNetPackets.h"

namespace ola {
namespace plugin {
namespace sandnet {

using ola::network::IPV4Address;
using ola::network::UdpSocket;

class SandNetNode {
  public:
    typedef enum {
      SANDNET_PORT_MODE_DISABLED,
      SANDNET_PORT_MODE_OUT,
      SANDNET_PORT_MODE_IN,
      SANDNET_PORT_MODE_MOUT,
      SANDNET_PORT_MODE_MIN
    } sandnet_port_type;

    explicit SandNetNode(const string &preferred_ip);
    ~SandNetNode();

    const ola::network::Interface &GetInterface() const {
      return m_interface;
    }

    void SetName(const string &name) {
      m_node_name = name;
    }
    bool Start();
    bool Stop();
    std::vector<UdpSocket*> GetSockets();
    void SocketReady(UdpSocket *socket);

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
    string m_node_name;
    string m_preferred_ip;

    sandnet_port m_ports[SANDNET_MAX_PORTS];
    universe_handlers m_handlers;
    ola::network::Interface m_interface;
    UdpSocket m_control_socket;
    UdpSocket m_data_socket;
    RunLengthEncoder m_encoder;
    IPV4Address m_control_addr;
    IPV4Address m_data_addr;

    static const uint16_t CONTROL_PORT = 37895;
    static const uint16_t DATA_PORT = 37900;
    static const char CONTROL_ADDRESS[];
    static const char DATA_ADDRESS[];
    static const char DEFAULT_NODE_NAME[];
    static const uint32_t FIRMWARE_VERSION = 0x00050501;
};
}  // sandnet
}  // plugin
}  // ola
#endif  // PLUGINS_SANDNET_SANDNETNODE_H_
