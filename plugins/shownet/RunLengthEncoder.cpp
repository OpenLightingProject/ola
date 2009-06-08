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
 * RunLengthEncoder.cpp
 * The Run Length Encoder
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <RunLengthEncoder.h>

namespace lla {
namespace shownet {

/*
 * Take a DMXBuffer and RunLengthEncode the data
 * @param src the DmxBuffer with the DMX data
 * @param data where to store the RLE data
 * @param size the size of the data segment, set to the amount of data encoded
 * @return true if we encoded all data, false if we ran out of space
 */
bool RunLengthEncoder::Encode(const DmxBuffer &src,
                              uint8_t *data,
                              unsigned int &data_size) {

  unsigned int src_size = src.Size();
  unsigned int dst_size = data_size;
  unsigned int &dst_index = data_size;
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


/*
 * Decode the RLE'ed data into a DmxBuffer.
 * @param dst the DmxBuffer to store the result
 * @param start_channel the first channel for the RLE'ed data
 * @param src_data the data to decode
 * @param length the length of the data to decode
 */
bool RunLengthEncoder::Decode(DmxBuffer &dst,
                              unsigned int start_channel,
                              uint8_t *src_data,
                              unsigned int length) {
  int destination_index = start_channel;

  for (unsigned int i = 0; i < length;) {
    unsigned int segment_length = src_data[i] & (~REPEAT_FLAG);
    if (src_data[i] & REPEAT_FLAG) {
      i++;
      dst.SetRangeToValue(destination_index, src_data[i++], segment_length);
    } else {
      i++;
      dst.SetRange(destination_index, src_data + i, segment_length);
      i += segment_length;
    }
    destination_index += segment_length;
  }
  return true;
}

} //shownet
} //lla
