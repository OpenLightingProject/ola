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
 * RDMFrame.cpp
 * The RDMFrame object.
 * Copyright (C) 2015 Simon Newton
 */

#include "ola/rdm/RDMFrame.h"

#include <ola/rdm/RDMPacket.h>
#include <string.h>
#include <vector>

namespace ola {
namespace rdm {

RDMFrame::RDMFrame(const uint8_t *raw_data, unsigned int length,
                   const Options &options) {
  if (options.prepend_start_code) {
    data.push_back(START_CODE);
  }
  data.append(raw_data, length);
  memset(reinterpret_cast<uint8_t*>(&timing), 0, sizeof(timing));
}

RDMFrame::RDMFrame(const ola::io::ByteString &frame_data,
                   const Options &options) {
  if (options.prepend_start_code) {
    data.push_back(START_CODE);
  }
  data.append(frame_data);
  memset(reinterpret_cast<uint8_t*>(&timing), 0, sizeof(timing));
}

bool RDMFrame::operator==(const RDMFrame &other) const {
  return (data == other.data &&
          timing.response_time == other.timing.response_time &&
          timing.break_time == other.timing.break_time &&
          timing.mark_time == other.timing.mark_time &&
          timing.data_time == other.timing.data_time);
}
}  // namespace rdm
}  // namespace ola
