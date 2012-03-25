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
 * E133Header.h
 * The E1.33 Header
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_E131_E131_E133HEADER_H_
#define PLUGINS_E131_E131_E133HEADER_H_

#include <stdint.h>
#include <string>

namespace ola {
namespace plugin {
namespace e131 {

using std::string;

/*
 * Header for the E133 layer
 */
class E133Header {
  public:
    E133Header() {}
    E133Header(const string &source,
               uint32_t sequence,
               uint16_t endpoint,
               bool rx_ack = false,
               bool timeout = false)
        : m_source(source),
          m_sequence(sequence),
          m_endpoint(endpoint),
          m_rx_ack(rx_ack),
          m_timeout(timeout) {
    }
    ~E133Header() {}

    const string Source() const { return m_source; }
    uint32_t Sequence() const { return m_sequence; }
    uint16_t Endpoint() const { return m_endpoint; }
    bool RxAcknowledge() const { return m_rx_ack; }
    bool Timeout() const { return m_timeout; }

    bool operator==(const E133Header &other) const {
      return m_source == other.m_source &&
        m_sequence == other.m_sequence &&
        m_endpoint == other.m_endpoint &&
        m_rx_ack == other.m_rx_ack &&
        m_timeout == other.m_timeout;
    }

    enum { SOURCE_NAME_LEN = 64 };

    struct e133_pdu_header_s {
      char source[SOURCE_NAME_LEN];
      uint32_t sequence;
      uint16_t endpoint;
      uint8_t options;
    } __attribute__((packed));
    typedef struct e133_pdu_header_s e133_pdu_header;

    static const uint8_t E133_RX_ACK_MASK = 0x80;
    static const uint8_t E133_TIMEOUT_MASK = 0x40;

  private:
    string m_source;
    uint32_t m_sequence;
    uint16_t m_endpoint;
    bool m_rx_ack;
    bool m_timeout;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_E133HEADER_H_
