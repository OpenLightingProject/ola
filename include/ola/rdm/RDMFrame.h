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
 * RDMFrame.h
 * Represents the raw contents of an RDM frame.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RDMFRAME_H_
#define INCLUDE_OLA_RDM_RDMFRAME_H_

#include <stdint.h>
#include <ola/io/ByteString.h>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

/**
 * @brief The raw data for a RDM message and its associated timing information.
 *
 * A RDMFrame holds the raw data and timing metadata for a RDM message. If no
 * timing data was available, the timing values will be 0.
 *
 * RDMFrames include the RDM Start Code (0xCC).
 */
class RDMFrame {
 public:
  struct Options {
   public:
    Options() : prepend_start_code(false) {}

    explicit Options(bool _prepend_start_code)
        : prepend_start_code(_prepend_start_code) {
    }

    /**
     * @brief True if the source data does not include a start code.
     */
    bool prepend_start_code;
  };

  /**
   * @brief Create an RDMFrame from a uint8_t pointer.
   * @param data A pointer to the frame data, the contents are copied.
   * @param length The size of the frame data.
   * @param options Options used when creating the RDMFrame.
   */
  RDMFrame(const uint8_t *data, unsigned int length,
           const Options &options = Options());

  /**
   * @brief Create a RDMFrame using a ByteString.
   * string.
   * @param data The ByteString with the RDM frame data.
   * @param options Options used when creating the RDMFrame.
   */
  explicit RDMFrame(const ola::io::ByteString &data,
                    const Options &options = Options());

  /**
   * @brief Test for equality.
   * @param other The RDMFrame to test against.
   * @returns True if two RDMFrames are equal, including any timing
   *   information.
   */
  bool operator==(const RDMFrame &other) const;

  /**
   * @brief The raw RDM data, including the RDM start code.
   */
  ola::io::ByteString data;

  /**
   * @brief The timing measurements for an RDM Frame.
   *
   * All times are measured in nanoseconds.
   *
   * For DUB responses, the break and mark values will be 0.
   */
  struct {
    /**
     * @brief The time between the end of the last byte of the request to the
     * start of the response.
     */
    uint32_t response_time;

    uint32_t break_time;  //!< The duration of the break.
    uint32_t mark_time;  //!< The duration of the mark-after-break.
    /**
     * @brief The time between the first and last byte of the data.
     */
    uint32_t data_time;
  } timing;
};

/**
 * @brief A vector of RDMFrames.
 */
typedef std::vector<RDMFrame> RDMFrames;

}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RDMFRAME_H_
