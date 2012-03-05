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
 * E131Node.h
 * Header file for the E131Node class, this is the interface between OLA and
 * the E1.31 library.
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef PLUGINS_E131_E131_E131NODE_H_
#define PLUGINS_E131_E131_E131NODE_H_

#include <map>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/network/Interface.h"
#include "ola/network/Socket.h"
#include "plugins/e131/e131/ACNPort.h"
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/E131Layer.h"
#include "plugins/e131/e131/E131Inflator.h"
#include "plugins/e131/e131/RootLayer.h"
#include "plugins/e131/e131/UDPTransport.h"
#include "plugins/e131/e131/DMPE131Inflator.h"

namespace ola {
namespace plugin {
namespace e131 {

class E131Node {
  public:
    E131Node(const std::string &ip_address,
             const CID &cid = CID::Generate(),
             bool use_rev2 = false,
             bool ignore_preview = true,
             uint8_t dscp_value = 0,  // default off
             uint16_t port = ACN_PORT);
    ~E131Node();

    bool Start();
    bool Stop();

    bool SetSourceName(unsigned int universe, const string &source);
    bool SendDMX(uint16_t universe,
                 const ola::DmxBuffer &buffer,
                 uint8_t priority = DEFAULT_PRIORITY,
                 bool preview = false);

    // The following method is provided for the testing framework. Don't use
    // it in production code!
    bool SendDMXWithSequenceOffset(uint16_t universe,
                                   const ola::DmxBuffer &buffer,
                                   int8_t sequence_offset,
                                   uint8_t priority = DEFAULT_PRIORITY,
                                   bool preview = false);

    bool StreamTerminated(uint16_t universe,
                          const ola::DmxBuffer &buffer = DmxBuffer(),
                          uint8_t priority = DEFAULT_PRIORITY);

    bool SetHandler(unsigned int universe, ola::DmxBuffer *buffer,
                    uint8_t *priority, ola::Callback0<void> *handler);
    bool RemoveHandler(unsigned int universe);

    const ola::network::Interface &GetInterface() const { return m_interface; }

    ola::network::UdpSocket* GetSocket() { return &m_socket; }

  private:
    typedef struct {
      string source;
      uint8_t sequence;
    } tx_universe;

    string m_preferred_ip;
    ola::network::Interface m_interface;
    ola::network::UdpSocket m_socket;
    CID m_cid;
    bool m_use_rev2;
    uint8_t m_dscp;
    uint16_t m_udp_port;
    // senders
    RootLayer m_root_layer;
    E131Layer m_e131_layer;
    // inflators
    RootInflator m_root_inflator;
    E131Inflator m_e131_inflator;
    E131InflatorRev2 m_e131_rev2_inflator;
    DMPE131Inflator m_dmp_inflator;

    IncomingUDPTransport m_incoming_udp_transport;
    std::map<unsigned int, tx_universe> m_tx_universes;
    uint8_t *m_send_buffer;

    tx_universe *SetupOutgoingSettings(unsigned int universe);

    E131Node(const E131Node&);
    E131Node& operator=(const E131Node&);

    static const uint16_t DEFAULT_PRIORITY = 100;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_E131NODE_H_
