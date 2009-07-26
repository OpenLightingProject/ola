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
 * ShowNetNode.h
 * Header file for the ShowNetNode class
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef OLA_SHOWNET_NODE
#define OLA_SHOWNET_NODE

#include <string>
#include <map>
#include <ola/Closure.h>
#include <ola/DmxBuffer.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/Socket.h>
#include "RunLengthEncoder.h"
#include "ShowNetPackets.h"

namespace ola {
namespace shownet {

class ShowNetNode {
  public :
    ShowNetNode(const std::string &ip_address);
    virtual ~ShowNetNode();

    bool Start();
    bool Stop();
    void SetName(const std::string &name);

    bool SendDMX(unsigned int universe, const ola::DmxBuffer &buffer);
    DmxBuffer GetDMX(unsigned int universe);
    bool SetHandler(unsigned int universe, ola::Closure *handler);
    bool RemoveHandler(unsigned int universe);

    ola::network::UdpSocket* GetSocket() { return m_socket; }
    int SocketReady();

    static const unsigned short SHOWNET_MAX_UNIVERSES = 8;

    friend class ShowNetNodeTest;

  private:
    typedef struct {
      DmxBuffer buffer;
      Closure *closure;
    } universe_handler;

    bool m_running;
    uint16_t m_packet_count;
    std::string m_node_name;
    std::string m_preferred_ip;
    std::map<unsigned int, universe_handler> m_handlers;
    ola::network::InterfacePicker m_interface_picker;
    ola::network::Interface m_interface;
    RunLengthEncoder m_encoder;
    ola::network::UdpSocket *m_socket;
    struct sockaddr_in m_destination;

    ShowNetNode(const ShowNetNode&);
    ShowNetNode& operator=(const ShowNetNode&);
    bool HandlePacket(const shownet_data_packet &packet, unsigned int size);
    unsigned int PopulatePacket(shownet_data_packet &packet,
                                unsigned int universe,
                                const DmxBuffer &buffer);
    bool InitNetwork();
    inline uint8_t ShortGetHigh(uint16_t x) const { return (0xff00 & x) >> 8; }
    inline uint8_t ShortGetLow(uint16_t x) const { return 0x00ff & x; }

    static const unsigned short SHOWNET_PORT = 2501;
    static const uint8_t SHOWNET_ID_HIGH = 0x80;
    static const uint8_t SHOWNET_ID_LOW = 0x8f;
    static const int MAGIC_INDEX_OFFSET = 11;
};

} //shownet
} //ola
#endif
