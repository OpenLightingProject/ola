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
 * RunLengthEncoder.cpp
 * The Run Length Encoder
 * Copyright (C) 2005 Simon Newton
 */

#include <string.h>
#include <ola/dmx/RunLengthEncoder.h>

namespace ola {
namespace dmx {

bool RunLengthEncoder::Encode(const DmxBuffer &src,
                              uint8_t *data,
                              unsigned int *data_size) {
  unsigned int src_size = src.Size();
  unsigned int dst_size = *data_size;
  unsigned int &dst_index = *data_size;
  dst_index = 0;

  unsigned int i;
  for (i = 0; i < src_size && dst_index < dst_size;) {
    // j points to the first non-repeating value
    unsigned int j = i + 1;
    while (j < src_size && src.Get(i) == src.Get(j) && j - i < 0x7f) {
      j++;
    }

    // if the number of repeats is more than 2
    // don't encode only two repeats,
    if (j - i > 2) {
      // if room left in dst buffer
      if (dst_size - dst_index > 1) {
        data[dst_index++] = (REPEAT_FLAG | (j - i));
        data[dst_index++] = src.Get(i);
      } else {
        // else return what we have done so far
        return false;
      }
      i = j;

    } else {
      // this value doesn't repeat more than twice
      // find out where the next repeat starts

      // postcondition: j is one more than the last value we want to send
      for (j = i + 1; j < src_size - 2 && j - i < 0x7f; j++) {
        // at the end  of the array
        if (j == src_size - 2) {
          j = src_size;
          break;
        }

        // if we're found a repeat of 3 or more stop here
        if (src.Get(j) == src.Get(j+1) && src.Get(j) == src.Get(j+2))
          break;
      }
      if (j >= src_size - 2)
        j = src_size;

      // if we have enough room left for all the values
      if (dst_index + j - i < dst_size) {
        data[dst_index++] = j - i;
        memcpy(&data[dst_index], src.GetRaw() + i, j-i);
        dst_index += j - i;
        i = j;

      // see how much data we can get in
      } else if (dst_size - dst_index > 1) {
        unsigned int l = dst_size - dst_index -1;
        data[dst_index++] = l;
        memcpy(&data[dst_index], src.GetRaw() + i, l);
        dst_index += l;
        return false;
      } else {
        return false;
      }
    }
  }

  if (i < src_size)
    return false;
  else
    return true;
}

bool RunLengthEncoder::Decode(unsigned int start_channel,
                              const uint8_t *src_data,
                              unsigned int length,
                              DmxBuffer *dst) {
  int destination_index = start_channel;

  for (unsigned int i = 0; i < length;) {
    unsigned int segment_length = src_data[i] & (~REPEAT_FLAG);
    if (src_data[i] & REPEAT_FLAG) {
      i++;
      dst->SetRangeToValue(destination_index, src_data[i++], segment_length);
    } else {
      i++;
      dst->SetRange(destination_index, src_data + i, segment_length);
      i += segment_length;
    }
    destination_index += segment_length;
  }
  return true;
}
}  // namespace dmx
}  // namespace ola
