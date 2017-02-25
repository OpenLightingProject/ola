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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * E131Header.h
 * The E1.31 Header
 * Copyright (C) 2007 Simon Newton
 */

#ifndef LIBS_ACN_E131HEADER_H_
#define LIBS_ACN_E131HEADER_H_

#include <ola/base/Macro.h>

#include <stdint.h>
#include <string>

namespace ola {
namespace acn {

/*
 * Header for the E131 layer
 */
class E131Header {
 public:
    E131Header()
        : m_priority(0),
          m_sequence(0),
          m_universe(0),
          m_is_preview(false),
          m_has_terminated(false),
          m_is_rev2(false) {
    }
    E131Header(const std::string &source,
               uint8_t priority,
               uint8_t sequence,
               uint16_t universe,
               bool is_preview = false,
               bool has_terminated = false,
               bool is_rev2 = false)
        : m_source(source),
          m_priority(priority),
          m_sequence(sequence),
          m_universe(universe),
          m_is_preview(is_preview),
          m_has_terminated(has_terminated),
          m_is_rev2(is_rev2) {
    }
    ~E131Header() {}

    const std::string Source() const { return m_source; }
    uint8_t Priority() const { return m_priority; }
    uint8_t Sequence() const { return m_sequence; }
    uint16_t Universe() const { return m_universe; }
    bool PreviewData() const { return m_is_preview; }
    bool StreamTerminated() const { return m_has_terminated; }

    bool UsingRev2() const { return m_is_rev2; }

    bool operator==(const E131Header &other) const {
      return m_source == other.m_source &&
        m_priority == other.m_priority &&
        m_sequence == other.m_sequence &&
        m_universe == other.m_universe &&
        m_is_preview == other.m_is_preview &&
        m_has_terminated == other.m_has_terminated &&
        m_is_rev2 == other.m_is_rev2;
    }

    enum { SOURCE_NAME_LEN = 64 };

    PACK(
    struct e131_pdu_header_s {
      char source[SOURCE_NAME_LEN];
      uint8_t priority;
      uint16_t reserved;
      uint8_t sequence;
      uint8_t options;
      uint16_t universe;
    });
    typedef struct e131_pdu_header_s e131_pdu_header;

    static const uint8_t PREVIEW_DATA_MASK = 0x80;
    static const uint8_t STREAM_TERMINATED_MASK = 0x40;

 private:
    std::string m_source;
    uint8_t m_priority;
    uint8_t m_sequence;
    uint16_t m_universe;
    bool m_is_preview;
    bool m_has_terminated;
    bool m_is_rev2;
};


class E131Rev2Header: public E131Header {
 public:
    E131Rev2Header(const std::string &source,
                   uint8_t priority,
                   uint8_t sequence,
                   uint16_t universe,
                   bool is_preview = false,
                   bool has_terminated = false)
        : E131Header(source, priority, sequence, universe, is_preview,
                     has_terminated, true) {
    }

    enum { REV2_SOURCE_NAME_LEN = 32 };

    typedef struct {
      char source[REV2_SOURCE_NAME_LEN];
      uint8_t priority;
      uint8_t sequence;
      uint16_t universe;
    } e131_rev2_pdu_header;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_E131HEADER_H_
