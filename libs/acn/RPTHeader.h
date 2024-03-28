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
 * RPTHeader.h
 * The E1.33 RPT Header
 * Copyright (C) 2023 Peter Newman
 */

#ifndef LIBS_ACN_RPTHEADER_H_
#define LIBS_ACN_RPTHEADER_H_

#include <ola/base/Macro.h>
#include <ola/rdm/UID.h>

#include <stdint.h>
#include <string>

namespace ola {
namespace acn {

/*
 * Header for the E133 RPT layer
 */
class RPTHeader {
 public:
  RPTHeader()
      : m_source_uid(0, 0),
        m_source_endpoint(0),
        m_destination_uid(0, 0),
        m_destination_endpoint(0),
        m_sequence(0) {
  }

  RPTHeader(ola::rdm::UID source_uid,
            uint16_t source_endpoint,
            ola::rdm::UID destination_uid,
            uint16_t destination_endpoint,
            uint32_t sequence)
      : m_source_uid(source_uid),
        m_source_endpoint(source_endpoint),
        m_destination_uid(destination_uid),
        m_destination_endpoint(destination_endpoint),
        m_sequence(sequence) {
  }
  ~RPTHeader() {}

  ola::rdm::UID SourceUID() const { return m_source_uid; }
  uint16_t SourceEndpoint() const { return m_source_endpoint; }
  ola::rdm::UID DestinationUID() const { return m_destination_uid; }
  uint16_t DestinationEndpoint() const { return m_destination_endpoint; }
  // TODO(Peter): Should this be SequenceNumber?
  // TODO(Peter): Should the sequence number really be part of the header?
  uint32_t Sequence() const { return m_sequence; }

  bool operator==(const RPTHeader &other) const {
    return m_source_uid == other.m_source_uid &&
      m_source_endpoint == other.m_source_endpoint &&
      m_destination_uid == other.m_destination_uid &&
      m_destination_endpoint == other.m_destination_endpoint &&
      m_sequence == other.m_sequence;
  }

  PACK(
  struct rpt_pdu_header_s {
    rpt_pdu_header_s() : reserved(0) {}
    uint8_t source_uid[ola::rdm::UID::LENGTH];
    uint16_t source_endpoint;
    uint8_t destination_uid[ola::rdm::UID::LENGTH];
    uint16_t destination_endpoint;
    uint32_t sequence;
    uint8_t reserved;
  });
  typedef struct rpt_pdu_header_s rpt_pdu_header;

 private:
  ola::rdm::UID m_source_uid;
  uint16_t m_source_endpoint;
  ola::rdm::UID m_destination_uid;
  uint16_t m_destination_endpoint;
  uint32_t m_sequence;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_RPTHEADER_H_
