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

#ifndef LLA_SHOWNET_NODE
#define LLA_SHOWNET_NODE

#include <string>
#include <map>
#include <lla/Closure.h>
#include <lla/DmxBuffer.h>
#include <lla/network/InterfacePicker.h>
#include <lla/network/Socket.h>

namespace lla {
namespace shownet {

class ShowNetNode {
  public :
    ShowNetNode(const std::string &ip_address);
    virtual ~ShowNetNode();

    bool Start();
    bool Stop();
    void SetName(const std::string &name);

    bool SendDMX(unsigned int universe, const lla::DmxBuffer &buffer);
    DmxBuffer GetDMX(unsigned int universe);
    bool SetHandler(unsigned int universe, lla::Closure *handler);
    bool RemoveHandler(unsigned int universe);

    lla::network::UdpSocket* GetSocket() { return m_socket; }
    int SocketReady();

    static const unsigned short SHOWNET_MAX_UNIVERSES = 8;

  private:
    typedef struct {
      DmxBuffer buffer;
      Closure *closure;
    } universe_handler;

    ShowNetNode(const ShowNetNode&);
    ShowNetNode& operator=(const ShowNetNode&);
    static const unsigned short SHOWNET_PORT = 2501;
    static const uint8_t SHOWNET_ID_HIGH = 0x80;
    static const uint8_t SHOWNET_ID_LOW = 0x8f;

    bool InitNetwork();
    inline uint8_t ShortGetHigh(uint16_t x) const { return (0xff00 & x) >> 8; }
    inline uint8_t ShortGetLow(uint16_t x) const { return 0x00ff & x; }

    bool m_running;
    uint16_t m_packet_count;
    std::string m_node_name;
    std::string m_preferred_ip;
    std::map<unsigned int, universe_handler> m_handlers;
    lla::network::InterfacePicker m_interface_picker;
    lla::network::Interface m_interface;
    class RunLengthEncoder *m_encoder;
    lla::network::UdpSocket *m_socket;
    struct sockaddr_in m_destination;

};

} //shownet
} //lla
#endif
