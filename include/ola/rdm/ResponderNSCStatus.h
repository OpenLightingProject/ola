/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * ResponderNSCStatus.h
 * Manages the NSC status for a RDM responder.
 * Copyright (C) 2025 Peter Newman
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file ResponderNSCStatus.h
 * @brief Manages the information about NSC status.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RESPONDERNSCSTATUS_H_
#define INCLUDE_OLA_RDM_RESPONDERNSCSTATUS_H_

#include <ola/rdm/RDMEnums.h>
#include <ola/DmxBuffer.h>
#include <stdint.h>

#include <algorithm>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

/**
 * @brief Holds information about NSC status.
 */
class NSCStatus {
 public:
  struct NSCStatusOptions {
   public:
    bool additive_checksum_support;
    bool packet_count_support;
    bool most_recent_slot_count_support;
    bool min_slot_count_support;
    bool max_slot_count_support;
    bool packet_error_count_support;

    // NSCStatusOptions constructor to set all options, for use in
    // initialisation lists. This also sets the defaults if called with no
    // args
    NSCStatusOptions(bool _additive_checksum_support = true,
                     bool _packet_count_support = true,
                     bool _most_recent_slot_count_support = true,
                     bool _min_slot_count_support = true,
                     bool _max_slot_count_support = true,
                     bool _packet_error_count_support = false)
        : additive_checksum_support(_additive_checksum_support),
          packet_count_support(_packet_count_support),
          most_recent_slot_count_support(_most_recent_slot_count_support),
          min_slot_count_support(_min_slot_count_support),
          max_slot_count_support(_max_slot_count_support),
          packet_error_count_support(_packet_error_count_support) {
    }
  };

  explicit NSCStatus(const NSCStatusOptions &options)
      : m_additive_checksum_support(options.additive_checksum_support),
        m_packet_count_support(options.packet_count_support),
        m_most_recent_slot_count_support(
            options.most_recent_slot_count_support),
        m_min_slot_count_support(options.min_slot_count_support),
        m_max_slot_count_support(options.max_slot_count_support),
        m_packet_error_count_support(options.packet_error_count_support),
        m_additive_checksum(0),
        m_packet_count(0),
        m_most_recent_slot_count(0),
        m_min_slot_count(0),
        m_max_slot_count(0),
        m_packet_error_count(0) {
  }
  virtual ~NSCStatus() {}

  uint32_t AdditiveChecksum() const {
    if (m_additive_checksum_support) {
      return m_additive_checksum;
    } else {
      return NSC_STATUS_ADDITIVE_CHECKSUM_UNSUPPORTED;
    }
  }

  uint32_t PacketCount() const {
    if (m_packet_count_support) {
      return m_packet_count;
    } else {
      return NSC_STATUS_PACKET_COUNT_UNSUPPORTED;
    }
  }

  uint16_t MostRecentSlotCount() const {
    if (m_most_recent_slot_count_support) {
      return m_most_recent_slot_count;
    } else {
      return NSC_STATUS_MOST_RECENT_SLOT_COUNT_UNSUPPORTED;
    }
  }

  uint16_t MinSlotCount() const {
    if (m_min_slot_count_support) {
      return m_min_slot_count;
    } else {
      return NSC_STATUS_MIN_SLOT_COUNT_UNSUPPORTED;
    }
  }

  uint16_t MaxSlotCount() const {
    if (m_max_slot_count_support) {
      return m_max_slot_count;
    } else {
      return NSC_STATUS_MAX_SLOT_COUNT_UNSUPPORTED;
    }
  }

  uint32_t PacketErrorCount() const {
    if (m_packet_error_count_support) {
      return m_packet_error_count;
    } else {
      return NSC_STATUS_PACKET_ERROR_COUNT_UNSUPPORTED;
    }
  }

  /**
   * @brief Update the statistics we can from the DmxBuffer
   * @note A DmxBuffer can only contain 512 slots (plus the start code) so this
   * will limit some edge cases.
   * @sa ReportError()
   */
  void UpdateStats(const DmxBuffer &buffer) {
    // Size() + 1 to account for the start code

    m_additive_checksum = buffer.AdditiveChecksum();
    m_most_recent_slot_count = std::min(NSC_STATUS_MOST_RECENT_SLOT_COUNT_MAX,
                                        (uint16_t) (buffer.Size() + 1));

    if (m_packet_count == 0) {
      // We must set the buffer size explicitly here on the first packet or
      // we'd be permanently stuck at 0
      m_min_slot_count = std::min(NSC_STATUS_MIN_SLOT_COUNT_MAX,
                                  (uint16_t) (buffer.Size() + 1));
    } else {
      m_min_slot_count = std::min(NSC_STATUS_MIN_SLOT_COUNT_MAX,
                                  std::min(m_min_slot_count,
                                           (uint16_t) (buffer.Size() + 1)));
    }

    m_max_slot_count = std::min(NSC_STATUS_MAX_SLOT_COUNT_MAX,
                                std::max(m_max_slot_count,
                                         (uint16_t) (buffer.Size() + 1)));

    // Update packet counter last so we can use it to track whether this is the
    // first packet or not
    if (m_packet_count < NSC_STATUS_PACKET_COUNT_MAX) {
      m_packet_count++;
    }

    // We can't establish error states from the buffer, so don't touch them
    // here
  }

  /**
   * @brief Report a NSC error to increment that counter
   */
  void ReportError() {
    if (m_packet_error_count < NSC_STATUS_PACKET_ERROR_COUNT_MAX) {
      m_packet_error_count++;
    }
  }

  /**
   * @brief Reset the NSC stats.
   */
  void Reset() {
    m_additive_checksum = 0;
    m_packet_count = 0;
    m_most_recent_slot_count = 0;
    m_min_slot_count = 0;
    m_max_slot_count = 0;
    m_packet_error_count = 0;
  }

  uint8_t SupportedFieldsBitMask() const {
    uint8_t bit_mask = 0;

    if (m_additive_checksum_support) {
      bit_mask |= NSC_STATUS_ADDITIVE_CHECKSUM_SUPPORTED_VALUE;
    }
    if (m_packet_count_support) {
      bit_mask |= NSC_STATUS_PACKET_COUNT_SUPPORTED_VALUE;
    }
    if (m_most_recent_slot_count_support) {
      bit_mask |= NSC_STATUS_MOST_RECENT_SLOT_COUNT_SUPPORTED_VALUE;
    }
    if (m_min_slot_count_support) {
      bit_mask |= NSC_STATUS_MIN_SLOT_COUNT_SUPPORTED_VALUE;
    }
    if (m_max_slot_count_support) {
      bit_mask |= NSC_STATUS_MAX_SLOT_COUNT_SUPPORTED_VALUE;
    }
    if (m_packet_error_count_support) {
      bit_mask |= NSC_STATUS_PACKET_ERROR_COUNT_SUPPORTED_VALUE;
    }
    return bit_mask;
  }

 protected:
  const bool m_additive_checksum_support;
  const bool m_packet_count_support;
  const bool m_most_recent_slot_count_support;
  const bool m_min_slot_count_support;
  const bool m_max_slot_count_support;
  const bool m_packet_error_count_support;
  uint32_t m_additive_checksum;
  uint32_t m_packet_count;
  uint16_t m_most_recent_slot_count;
  uint16_t m_min_slot_count;
  uint16_t m_max_slot_count;
  uint32_t m_packet_error_count;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERNSCSTATUS_H_
