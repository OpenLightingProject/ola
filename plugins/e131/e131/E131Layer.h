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
 * E131Layer.h
 * Interface for the E131Layer class, this abstracts the encapsulation and
 * sending of DMP PDUs contained within E131PDUs as well as the setting of
 * the DMP inflator inflator.
 * Copyright (C) 2007 Simon Newton
 */

#ifndef PLUGINS_E131_E131_E131LAYER_H_
#define PLUGINS_E131_E131_E131LAYER_H_

#include "ola/network/Socket.h"
#include "plugins/e131/e131/DMPPDU.h"
#include "plugins/e131/e131/E131Header.h"
#include "plugins/e131/e131/PreamblePacker.h"
#include "plugins/e131/e131/Transport.h"
#include "plugins/e131/e131/UDPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {

class DMPInflator;

class E131Layer {
  public:
    E131Layer(ola::network::UdpSocket *socket,
              class RootSender *root_layer);
    ~E131Layer() {}

    bool SendDMP(const E131Header &header, const DMPPDU *pdu);
    bool UniverseIP(unsigned int universe,
                    class ola::network::IPV4Address *addr);

  private:
    ola::network::UdpSocket *m_socket;
    PreamblePacker m_packer;
    OutgoingUDPTransportImpl m_transport_impl;
    class RootSender *m_root_sender;

    E131Layer(const E131Layer&);
    E131Layer& operator=(const E131Layer&);
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_E131LAYER_H_
