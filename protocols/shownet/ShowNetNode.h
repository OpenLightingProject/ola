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

#ifndef SHOWNET_NODE_H
#define SHOWNET_NODE_H

#include <string>
#include <map>
#include <lla/Closure.h>
#include <lla/DmxBuffer.h>

namespace lla {
namespace shownet {

class ShowNetNode {
  public :
    ShowNetNode(const std::string &ip_address);
    virtual ~ShowNetNode();

    bool Start();
    bool Stop();
    bool SetName(const std::string &name);

    bool SendDMX(unsigned int universe, const lla::DmxBuffer &buffer);
    bool SetHandler(unsigned int universe, lla:LlaClosure *handler);
    bool RemoveHandler(unsigned int universe);

    /*
    int shownet_read(shownet_node n, int timeout);

    // misc
    int shownet_get_sd(shownet_node n);
    */

  private:
    ShowNetNode(const ShowNetNode&);
    ShowNetNode& operator=(const ShowNetNode&);


    bool m_running;
    uint16_t m_packet_count;
    std::string m_node_name;
    std::map<unsigned int, LlaClosure*> m_handlers;
    InterfacePicker m_interface_picker;
    RunLengthEncoder m_encoder;

    SI ip_addr;
    SI bcast_addr;

    UdpListeningSocket *m_socket;
};

} //shownet
} //lla
#endif
