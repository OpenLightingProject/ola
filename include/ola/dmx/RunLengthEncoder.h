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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * RunLengthEncoder.h
 * Header file for the RunLengthEncoder class
 * Copyright (C) 2005-2009 Simon Newton
 */

/**
 * @file RunLengthEncoder.h
 * @brief Encode / Decode DMX data using [Run Length
 * Encoding](http://en.wikipedia.org/wiki/Run-length_encoding)
 */

#ifndef INCLUDE_OLA_RUNLENGTHENCODER_H_
#define INCLUDE_OLA_RUNLENGTHENCODER_H_

#include <ola/DmxBuffer.h>

namespace ola {
namespace dmx {

/**
 * @brief Encode / Decode DMX data using [Run Length
 * Encoding](http://en.wikipedia.org/wiki/Run-length_encoding)
 */
class RunLengthEncoder {
  public :
    RunLengthEncoder() {}
    ~RunLengthEncoder() {}

    /**
     * Given a DMXBuffer, run length encode the data.
     * @param[in] src the DmxBuffer to encode.
     * @param[out] data where to store the RLE data
     * @param[in,out] size the size of the data segment, set to the amount of
     * data encoded.
     * @return true if we encoded all data, false if we ran out of space
     */
    bool Encode(const DmxBuffer &src,
                uint8_t *data,
                unsigned int &size);

    /**
     * Decode an DMX frame and place the output in a DmxBuffer
     * @param[in] start_channel the first channel for the RLE'ed data
     * @param[in] data the encoded frame.
     * @param[in] length the length of the encoded frame.
     * @param[out] The DmxBuffer to store the frame in
     * @returns true if decoding was successful, false otherwise.
     */
    bool Decode(unsigned int start_channel,
                const uint8_t *data,
                unsigned int length,
                DmxBuffer *output);

  private:
    static const uint8_t REPEAT_FLAG = 0x80;
};
}  // dmx
}  // namespace ola
#endif  // INCLUDE_OLA_RUNLENGTHENCODER_H_
