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
 * RunLengthDecoder.cpp
 * The Run Length Decoder
 * Copyright (C) 2005 Simon Newton
 */

#include <ola/Constants.h>
#include <ola/base/Macro.h>
#include "plugins/espnet/RunLengthDecoder.h"

namespace ola {
namespace plugin {
namespace espnet {

/*
 * Decode the RLE'ed data into a DmxBuffer.
 * @param dst the DmxBuffer to store the result
 * @param src_data the data to decode
 * @param length the length of the data to decode
 */
void RunLengthDecoder::Decode(DmxBuffer *dst,
                              const uint8_t *src_data,
                              unsigned int length) {
  dst->Reset();
  unsigned int i = 0;
  const uint8_t *value = src_data;
  uint8_t count;
  while (i < DMX_UNIVERSE_SIZE && value < src_data + length) {
    switch (*value) {
      case REPEAT_VALUE:
        value++;
        count = *(value++);
        dst->SetRangeToValue(i, *value, count);
        i+= count;
        break;
      case ESCAPE_VALUE:
        value++;
        // fall through
        OLA_FALLTHROUGH
      default:
        dst->SetChannel(i, *value);
        i++;
    }
    value++;
  }
}
}  // namespace espnet
}  // namespace plugin
}  // namespace ola
