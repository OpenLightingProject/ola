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
 * E133Node.h
 * Copyright (C) 2011 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include HASH_MAP_H

#include <ola/network/SelectServer.h>
#include <string>

#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/DMPE133Inflator.h"
#include "plugins/e131/e131/E133Layer.h"
#include "plugins/e131/e131/RootLayer.h"
#include "plugins/e131/e131/UDPTransport.h"

#ifndef TOOLS_E133_E133NODE_H_
#define TOOLS_E133_E133NODE_H_

using std::string;

class E133Node {
  public:
    E133Node(const string &preferred_ip, uint16_t port);
    ~E133Node();

    bool Init();
    void Run() { m_ss.Run(); }
    void Stop() { m_ss.Terminate(); }

    bool RegisterComponent(class E133Component *component);
    void UnRegisterComponent(class E133Component *component);
    // bool RegisterReciever(E133UniverseReceiver *receiver);

    void HandleManagementPacket(
        const ola::plugin::e131::TransportHeader &transport_header,
        const ola::plugin::e131::E133Header &e133_header,
        const string &request);

    bool CheckForStaleRequests();


  private:
    typedef HASH_NAMESPACE::HASH_MAP_CLASS<
      unsigned int,
      class E133Component*> component_map;

    const string m_preferred_ip;
    ola::network::SelectServer m_ss;
    ola::network::Event *m_timeout_event;
    component_map m_component_map;

    ola::plugin::e131::CID m_cid;
    ola::plugin::e131::UDPTransport m_transport;
    ola::plugin::e131::RootLayer m_root_layer;
    ola::plugin::e131::E133Layer m_e133_layer;
    ola::plugin::e131::DMPE133Inflator m_dmp_inflator;
};
#endif  // TOOLS_E133_E133NODE_H_
