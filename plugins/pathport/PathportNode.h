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
 * PathportNode.h
 * Header file for the PathportNode class
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef PLUGINS_PATHPORT_PATHPORTNODE_H_
#define PLUGINS_PATHPORT_PATHPORTNODE_H_

#include <map>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/Socket.h"
#include "plugins/pathport/PathportPackets.h"

namespace ola {
namespace plugin {
namespace pathport {

using ola::network::IPV4Address;
using ola::network::UDPSocket;

class PathportNode {
  public:
    explicit PathportNode(const string &preferred_ip, uint32_t device_id,
                          uint8_t dscp);
    ~PathportNode();

    bool Start();
    bool Stop();
    const ola::network::Interface &GetInterface() const {
      return m_interface;
    }
    UDPSocket *GetSocket() { return &m_socket; }
    void SocketReady(UDPSocket *socket);

    bool SetHandler(uint8_t universe,
                    DmxBuffer *buffer,
                    Callback0<void> *closure);
    bool RemoveHandler(uint8_t universe);

    bool SendArpReply();
    bool SendDMX(unsigned int universe, const DmxBuffer &buffer);

    // apparently pathport supports up to 128 universes, the spec only says 64
    static const uint8_t MAX_UNIVERSES = 127;

  private:
    typedef struct {
      DmxBuffer *buffer;
      Callback0<void> *closure;
    } universe_handler;

    enum {
      XDMX_DATA_FLAT = 0x0101,
      XDMX_DATA_RELEASE = 0x0103
    };

    enum {
      NODE_MANUF_PATHWAY_CONNECTIVITY = 0,
      NODE_MANUF_INTERACTIVE_TECH = 0x10,
      NODE_MANUF_ENTERTAINMENT_TECH = 0x11,
      NODE_MANUF_MA_LIGHTING = 0x12,
      NODE_MANUF_HIGH_END_SYSTEMS = 0x13,
      NODE_MANUF_CRESTRON_ELECTRONICS = 0x14,
      NODE_MANUF_LEVITON = 0x15,
      NODE_MANUF_FLYING_PIG = 0x16,
      NODE_MANUF_HORIZON = 0x17,
      NODE_MANUF_ZP_TECH = 0x28,  // ola
    };

    enum {
      NODE_CLASS_DMX_NODE = 0,
      NODE_CLASS_MANAGER = 1,
      NODE_CLASS_DIMMER = 2,
      NODE_CLASS_CONTROLLER = 3,
      NODE_CLASS_FIXTURE = 4,
      NODE_CLASS_EFFECTS_UNIT = 5,
    };

    enum {
      NODE_DEVICE_PATHPORT = 0,
      NODE_DEVICE_DMX_MANAGER_PLUS = 1,
      NODE_DEVICE_ONEPORT = 2,
    };

    typedef std::map<uint8_t, universe_handler> universe_handlers;

    bool InitNetwork();
    void PopulateHeader(pathport_packet_header *header, uint32_t destination);
    bool ValidateHeader(const pathport_packet_header &header);
    void HandleDmxData(const pathport_pdu_data &packet,
                       unsigned int size);
    bool SendArpRequest(uint32_t destination = PATHPORT_ID_BROADCAST);
    bool SendPacket(const pathport_packet_s &packet,
                    unsigned int size,
                    IPV4Address dest);

    bool m_running;
    uint8_t m_dscp;
    string m_preferred_ip;
    uint32_t m_device_id;  // the pathport device id
    uint16_t m_sequence_number;

    universe_handlers m_handlers;
    ola::network::Interface m_interface;
    UDPSocket m_socket;
    IPV4Address m_config_addr;
    IPV4Address m_status_addr;
    IPV4Address m_data_addr;

    static const uint16_t PATHPORT_PORT = 0xed0;
    static const uint16_t PATHPORT_PROTOCOL = 0xed01;
    static const uint32_t PATHPORT_CONFIG_GROUP = 0xefffed02;
    static const uint32_t PATHPORT_DATA_GROUP = 0xefffed01;
    static const uint32_t PATHPORT_ID_BROADCAST = 0xffffffff;
    static const uint32_t PATHPORT_STATUS_GROUP = 0xefffedff;
    static const uint8_t MAJOR_VERSION = 2;
    static const uint8_t MINOR_VERSION = 0;
};
}  // namespace pathport
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_PATHPORT_PATHPORTNODE_H_
