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
 * DMPAddress.cpp
 * Defines the DMP property address types
 * Copyright (C) 2007 Simon Newton
 */

#include "ola/network/NetworkUtils.h"
#include "libs/acn/DMPAddress.h"

namespace ola {
namespace acn {

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
                                    unsigned int *length) {
  unsigned int byte_count = (type == NON_RANGE ? 1 : 3) *
                            DMPSizeToByteSize(size);

  if (size == RES_BYTES || *length < byte_count) {
    *length = 0;
    return NULL;
  }

  *length = byte_count;
  const uint8_t *addr1 = data;
  // We have to do a memcpy to avoid the word alignment issues on ARM
  uint16_t addr2[3];
  uint32_t addr4[3];

  if (type == NON_RANGE) {
    switch (size) {
      case ONE_BYTES:
        return new OneByteDMPAddress(*data);
      case TWO_BYTES:
        memcpy(addr2, data, sizeof(addr2));
        return new TwoByteDMPAddress(NetworkToHost(addr2[0]));
      case FOUR_BYTES:
        memcpy(addr4, data, sizeof(addr4));
        return new FourByteDMPAddress(NetworkToHost(addr4[0]));
      default:
        return NULL;  // should never make it here because we checked above
    }
  }

  switch (size) {
    case ONE_BYTES:
      return new OneByteRangeDMPAddress(addr1[0], addr1[1], addr1[2]);
    case TWO_BYTES:
      memcpy(addr2, data, sizeof(addr2));
      return new TwoByteRangeDMPAddress(NetworkToHost(addr2[0]),
                                        NetworkToHost(addr2[1]),
                                        NetworkToHost(addr2[2]));
    case FOUR_BYTES:
      memcpy(addr4, data, sizeof(addr4));
      return new FourByteRangeDMPAddress(NetworkToHost(addr4[0]),
                                         NetworkToHost(addr4[1]),
                                         NetworkToHost(addr4[2]));
    default:
      return NULL;  // should never make it here because we checked above
  }
}
}  // namespace acn
}  // namespace ola
