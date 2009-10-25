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
 * DMPAddress.cpp
 * Defines the DMP property address types
 * Copyright (C) 2007 Simon Newton
 */

#include <ola/network/NetworkUtils.h>
#include "DMPAddress.h"

namespace ola {
namespace e131 {

using ola::network::NetworkToHost;

/*
 * Return the number of bytes that correspond to a DMPType
 */
unsigned int DMPSizeToByteSize(dmp_address_size size) {
  switch (size) {
    case ONE_BYTES:
      return 1;
    case TWO_BYTES:
      return 2;
    case FOUR_BYTES:
      return 4;
    default:
      return 0;
  }
}

/*
 * Create a new single address
 */
const BaseDMPAddress *NewSingleAddress(unsigned int value) {
  if (value > MAX_TWO_BYTE)
    return new FourByteDMPAddress(value);
  else if (value > MAX_ONE_BYTE)
    return new TwoByteDMPAddress((uint16_t) value);
  return new OneByteDMPAddress((uint8_t) value);
}


/*
 * Create a new range address.
 */
const BaseDMPAddress *NewRangeAddress(unsigned int value,
                                      unsigned int increment,
                                      unsigned int number) {
  if (value > MAX_TWO_BYTE || increment > MAX_TWO_BYTE ||
      number > MAX_TWO_BYTE)
    return new FourByteRangeDMPAddress(value, increment, number);
  else if (value > MAX_ONE_BYTE || increment > MAX_ONE_BYTE ||
           number > MAX_ONE_BYTE)
    return new TwoByteRangeDMPAddress((uint16_t) value,
                                      (uint16_t) increment,
                                      (uint16_t) number);
  return new OneByteRangeDMPAddress((uint8_t) value,
                                    (uint8_t) increment,
                                    (uint8_t) number);
}


/*
 * Decode a block of data into a DMPAddress
 */
const BaseDMPAddress *DecodeAddress(dmp_address_size size,
                                    dmp_address_type type,
                                    const uint8_t *data,
                                    unsigned int &length) {
  unsigned int byte_count = (type == NON_RANGE ? 1 : 3) *
                            DMPSizeToByteSize(size);

  if (size == RES_BYTES || length < byte_count) {
    length = 0;
    return NULL;
  }

  length = byte_count;

  uint16_t *p = (uint16_t*) data;
  uint32_t *p2 = (uint32_t*) data;

  if (type == NON_RANGE) {
    switch (size) {
      case ONE_BYTES:
        return new OneByteDMPAddress(*data);
      case TWO_BYTES:
        return new TwoByteDMPAddress(NetworkToHost(*p));
      case FOUR_BYTES:
        return new FourByteDMPAddress(NetworkToHost(*p2));
      default:
        return NULL; // should never make it here because we checked above
    }
  }

  switch (size) {
    case ONE_BYTES:
      return new OneByteRangeDMPAddress(*data++, *data++, *data);
    case TWO_BYTES:
      return new TwoByteRangeDMPAddress(NetworkToHost(*p++),
                                        NetworkToHost(*p++),
                                        NetworkToHost(*p));
    case FOUR_BYTES:
      return new FourByteRangeDMPAddress(NetworkToHost(*p2++),
                                         NetworkToHost(*p2++),
                                         NetworkToHost(*p2));
    default:
      return NULL; // should never make it here because we checked above
  }
}


} // e131
} // ola
