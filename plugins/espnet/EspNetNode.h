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
 * EspNetNode.h
 * Header file for the EspNetNode class
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef PLUGINS_ESPNET_ESPNETNODE_H_
#define PLUGINS_ESPNET_ESPNETNODE_H_

#include <string>
#include <map>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Interface.h"
#include "ola/network/Socket.h"
#include "plugins/espnet/EspNetPackets.h"
#include "plugins/espnet/RunLengthDecoder.h"

namespace ola {
namespace plugin {
namespace espnet {

using ola::network::IPV4Address;

// the node types
typedef enum {
    ESPNET_NODE_TYPE_SINGLE_OUT = 0x0001,  // ip to dmx
    ESPNET_NODE_TYPE_SINGLE_IN = 0x0002,  // dmx to ip
    ESPNET_NODE_TYPE_RS232 = 0x0060,
    ESPNET_NODE_TYPE_IO = 0x0061,  // multi universe
    ESPNET_NODE_TYPE_LONWORKS = 0x0100,
} espnet_node_type;

enum { ESPNET_MAX_UNIVERSES = 512 };


class EspNetNode {
  public:
    explicit EspNetNode(const std::string &ip_address);
    virtual ~EspNetNode();

    bool Start();
    bool Stop();

    const ola::network::Interface &GetInterface() const {
      return m_interface;
    }

    void SetName(const std::string &name) { m_node_name = name; }
    void SetType(espnet_node_type type) { m_type = type; }
    void SetUniverse(uint8_t universe) { m_universe = universe; }

    // IO methods
    ola::network::UDPSocket* GetSocket() { return &m_socket; }
    void SocketReady();

    // DMX Receiving methods
    bool SetHandler(uint8_t universe, DmxBuffer *buffer,
                    ola::Callback0<void> *handler);
    bool RemoveHandler(uint8_t universe);

    // Sending methods
    bool SendPoll(bool full_poll = false);
    bool SendDMX(uint8_t universe, const ola::DmxBuffer &buffer);

  private:
    typedef struct {
      DmxBuffer *buffer;
      Callback0<void> *closure;
    } universe_handler;

    EspNetNode(const EspNetNode&);
    EspNetNode& operator=(const EspNetNode&);
    bool InitNetwork();
    void HandlePoll(const espnet_poll_t &poll, ssize_t length,
                    const IPV4Address &source);
    void HandleReply(const espnet_poll_reply_t &reply,
                     ssize_t length,
                     const IPV4Address &source);
    void HandleAck(const espnet_ack_t &ack, ssize_t length,
                   const IPV4Address &source);
    void HandleData(const espnet_data_t &data, ssize_t length,
                    const IPV4Address &source);

    bool SendEspPoll(const IPV4Address &dst, bool full);
    bool SendEspAck(const IPV4Address &dst,
                    uint8_t status,
                    uint8_t crc);
    bool SendEspPollReply(const IPV4Address &dst);
    bool SendEspData(const IPV4Address &dst,
                     uint8_t universe,
                     const DmxBuffer &buffer);
    bool SendPacket(const IPV4Address &dst,
                    const espnet_packet_union_t &packet,
                    unsigned int size);

    bool m_running;
    uint8_t m_options;
    uint8_t m_tos;
    uint8_t m_ttl;
    uint8_t m_universe;
    espnet_node_type m_type;
    std::string m_node_name;
    std::string m_preferred_ip;
    std::map<uint8_t, universe_handler> m_handlers;
    ola::network::Interface m_interface;
    ola::network::UDPSocket m_socket;
    RunLengthDecoder m_decoder;

    static const char NODE_NAME[];
    static const uint8_t DEFAULT_OPTIONS = 0;
    static const uint8_t DEFAULT_TOS = 0;
    static const uint8_t DEFAULT_TTL = 4;
    static const uint8_t FIRMWARE_VERSION = 1;
    static const uint8_t SWITCH_SETTINGS = 0;
    static const uint16_t ESPNET_PORT = 3333;
    static const uint8_t DATA_RAW = 1;
    static const uint8_t DATA_PAIRS = 2;
    static const uint8_t DATA_RLE = 4;
    static const uint8_t START_CODE = 0;
};
}  // namespace espnet
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_ESPNET_ESPNETNODE_H_
