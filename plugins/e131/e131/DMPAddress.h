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
 * DMPAddress.h
 * Defines the DMP property address types
 * Copyright (C) 2007 Simon Newton
 */

#ifndef OLA_E131_DMPADDRESS_H
#define OLA_E131_DMPADDRESS_H

#include <stdint.h>

namespace ola {
namespace e131 {

typedef enum {
  ONE_BYTES = 0x00,
  TWO_BYTES = 0x01,
  FOUR_BYTES = 0x02,
  RES_BYTES = 0x03
} dmp_address_size;

/*
 * The Base DMPAddress class.
 * The addresses represented by this class may be actual or virtual & relative
 * or absolute, single or repeated.
 */
class DMPAddress {
  public:
    DMPAddress() {}
    virtual ~DMPAddress() {}

    // The start address
    virtual unsigned int Start() const = 0;
    // The increment
    virtual unsigned int Increment() const = 0;
    // The number of properties referenced
    virtual unsigned int Number() const = 0;

    // Size of this address structure
    virtual unsigned int Size() const = 0;

    // Pack this address into memory
    virtual bool Pack(uint8_t *data, unsigned int &length) const = 0;

    // True if this is a repeated address.
    virtual bool IsRepeated() const = 0;
};


/*
 * These type of addresses only reference one property.
 */
template<typename type>
class SingleDMPAddress: public DMPAddress {
  public:
    SingleDMPAddress(type start):
      DMPAddress(),
      m_start(start) {}

    unsigned int Start() const { return m_start; }
    unsigned int Increment() const { return 0; }
    unsigned int Number() const { return 1; }
    unsigned int Size() const { return sizeof(type); }
    bool Pack(uint8_t *data, unsigned int &length) const {
      if (length < Size()) {
        length = 0;
        return false;
      }
      type *field = (type*) data;
      *field = m_start;
      length = Size();
      return true;
    }
    bool IsRepeated() const { return false; }

  private:
    type m_start;
};


typedef SingleDMPAddress<uint8_t> OneByteSingleDMPAddress;
typedef SingleDMPAddress<uint16_t> TwoByteSingleDMPAddress;
typedef SingleDMPAddress<uint32_t> FourByteSingleDMPAddress;

/*
 * Create a new single address
 */
DMPAddress *NewSingleAddress(unsigned int value) {
  if (value > 0xffff)
    return new FourByteSingleDMPAddress(value);
  else if (value > 0xff)
    return new TwoByteSingleDMPAddress(value);
  return new OneByteSingleDMPAddress(value);
}


/*
 * These type of addresses reference multiple properties.
 */
template <typename type>
class RepeatedDMPAddress: public DMPAddress {
  public:
    RepeatedDMPAddress(type start,
                       type increment,
                       type number):
      DMPAddress(),
      m_start(start),
      m_increment(increment),
      m_number(number) {}
    unsigned int Start() const { return m_start; }
    unsigned int Increment() const { return m_increment; }
    unsigned int Number() const { return m_number; }
    unsigned int Size() const { return 3 * sizeof(type); }
    bool Pack(uint8_t *data, unsigned int &length) const {
      if (length < Size()) {
        length = 0;
        return false;
      }
      type *field = (type*) data;
      *field++ = m_start;
      *field++ = m_increment;
      *field = m_number;
      length = Size();
      return true;
    }
    bool IsRepeated() const { return true; }

  private:
    type m_start, m_increment, m_number;
};


typedef RepeatedDMPAddress<uint8_t> OneByteRepeatedDMPAddress;
typedef RepeatedDMPAddress<uint16_t> TwoByteRepeatedDMPAddress;
typedef RepeatedDMPAddress<uint32_t> FourByteRepeatedDMPAddress;


/*
 * Create a new repeated address.
 */
DMPAddress *NewRepeatedAddress(unsigned int value,
                               unsigned int increment,
                               unsigned int number) {
  if (value > 0xffff)
    return new FourByteRepeatedDMPAddress(value, increment, number);
  else if (value > 0xff)
    return new TwoByteRepeatedDMPAddress(value, increment, number);
  return new OneByteRepeatedDMPAddress(value, increment, number);
}


/*
 * Return the dmp_address_size that corresponds to a type
 */
template <typename type>
dmp_address_size DMPTypeToSize() { assert(0); return RES_BYTES; }

template <>
dmp_address_size DMPTypeToSize<uint8_t>() { return ONE_BYTES; }

template <>
dmp_address_size DMPTypeToSize<uint16_t>() { return TWO_BYTES; }

template <>
dmp_address_size DMPTypeToSize<uint32_t>() { return FOUR_BYTES; }


} // e131
} // ola

#endif
