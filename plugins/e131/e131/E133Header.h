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
               uint8_t priority,
               uint8_t sequence,
               uint16_t universe,
               bool is_management = false,
               bool is_squawk = false)
        : m_source(source),
          m_priority(priority),
          m_sequence(sequence),
          m_universe(universe),
          m_is_management(is_management),
          m_is_squawk(is_squawk) {
    }
    ~E133Header() {}

    const string Source() const { return m_source; }
    uint8_t Priority() const { return m_priority; }
    uint8_t Sequence() const { return m_sequence; }
    uint16_t Universe() const { return m_universe; }
    bool IsManagement() const { return m_is_management; }
    bool IsSquawk() const { return m_is_squawk; }

    bool operator==(const E133Header &other) const {
      return m_source == other.m_source &&
        m_priority == other.m_priority &&
        m_sequence == other.m_sequence &&
        m_universe == other.m_universe &&
        m_is_management == other.m_is_management &&
        m_is_squawk == other.m_is_squawk;
    }

    enum { SOURCE_NAME_LEN = 64 };

    struct e133_pdu_header_s {
      char source[SOURCE_NAME_LEN];
      uint8_t priority;
      uint16_t reserved;
      uint8_t sequence;
      uint8_t options;
      uint16_t universe;
    } __attribute__((packed));
    typedef struct e133_pdu_header_s e133_pdu_header;

    static const uint8_t RDM_MANAGEMENT_MASK = 0x20;
    static const uint8_t RDM_SQUAWK_MASK = 0x10;

  private:
    string m_source;
    uint8_t m_priority;
    uint8_t m_sequence;
    uint16_t m_universe;
    bool m_is_management;
    bool m_is_squawk;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_E133HEADER_H_
