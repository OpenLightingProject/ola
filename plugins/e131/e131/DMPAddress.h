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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DMPAddress.h
 * Defines the DMP property address types
 * Copyright (C) 2007 Simon Newton
 */

#ifndef PLUGINS_E131_E131_DMPADDRESS_H_
#define PLUGINS_E131_E131_DMPADDRESS_H_

#include <stdint.h>
#include <string.h>
#include "ola/io/OutputStream.h"
#include "ola/network/NetworkUtils.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::HostToNetwork;

typedef enum {
  ONE_BYTES = 0x00,
  TWO_BYTES = 0x01,
  FOUR_BYTES = 0x02,
  RES_BYTES = 0x03
} dmp_address_size;


typedef enum {
  NON_RANGE = 0x00,
  RANGE_SINGLE = 0x01,
  RANGE_EQUAL = 0x02,
  RANGE_MIXED = 0x03,
} dmp_address_type;


static const unsigned int MAX_TWO_BYTE = 0xffff;
static const unsigned int MAX_ONE_BYTE = 0xff;


/*
 * Return the dmp_address_size that corresponds to a type
 */
template <typename type>
dmp_address_size TypeToDMPSize() {
  switch (sizeof(type)) {
    case 1:
      return ONE_BYTES;
    case 2:
      return TWO_BYTES;
    case 4:
      return FOUR_BYTES;
    default:
      return RES_BYTES;
  }
}


/*
 * Return the number of bytes that correspond to a DMPType
 */
unsigned int DMPSizeToByteSize(dmp_address_size size);


/*
 * The Base DMPAddress class.
 * The addresses represented by this class may be actual or virtual & relative
 * or absolute, ranged or non-ranged.
 */
class BaseDMPAddress {
  public:
    BaseDMPAddress() {}
    virtual ~BaseDMPAddress() {}

    // The start address
    virtual unsigned int Start() const = 0;
    // The increment
    virtual unsigned int Increment() const = 0;
    // The number of properties referenced
    virtual unsigned int Number() const = 0;

    // Size of this address structure
    virtual unsigned int Size() const {
      return (IsRange() ? 3 : 1) * BaseSize();
    }

    virtual dmp_address_size AddressSize() const = 0;

    // Pack this address into memory
    virtual bool Pack(uint8_t *data, unsigned int &length) const = 0;

    // Write this address to an OutputStream
    virtual void Write(ola::io::OutputStream *stream) const = 0;

    // True if this is a range address.
    virtual bool IsRange() const = 0;

  protected:
    virtual unsigned int BaseSize() const = 0;
};


/*
 * These type of addresses only reference one property.
 */
template<typename type>
class DMPAddress: public BaseDMPAddress {
  public:
    explicit DMPAddress(type start):
      BaseDMPAddress(),
      m_start(start) {}

    unsigned int Start() const { return m_start; }
    unsigned int Increment() const { return 0; }
    unsigned int Number() const { return 1; }
    dmp_address_size AddressSize() const { return TypeToDMPSize<type>(); }

    bool Pack(uint8_t *data, unsigned int &length) const {
      if (length < Size()) {
        length = 0;
        return false;
      }
      type field = HostToNetwork(m_start);
      memcpy(data, &field, BaseSize());
      length = Size();
      return true;
    }

    void Write(ola::io::OutputStream *stream) const {
      *stream << HostToNetwork(m_start);
    }

    bool IsRange() const { return false; }

  protected:
    unsigned int BaseSize() const { return sizeof(type); }

  private:
    type m_start;
};


typedef DMPAddress<uint8_t> OneByteDMPAddress;
typedef DMPAddress<uint16_t> TwoByteDMPAddress;
typedef DMPAddress<uint32_t> FourByteDMPAddress;

/*
 * Create a new single address
 */
const BaseDMPAddress *NewSingleAddress(unsigned int value);


/*
 * These type of addresses reference multiple properties.
 */
template <typename type>
class RangeDMPAddress: public BaseDMPAddress {
  public:
    RangeDMPAddress(type start,
                    type increment,
                    type number):
      BaseDMPAddress(),
      m_start(start),
      m_increment(increment),
      m_number(number) {}
    unsigned int Start() const { return m_start; }
    unsigned int Increment() const { return m_increment; }
    unsigned int Number() const { return m_number; }
    dmp_address_size AddressSize() const { return TypeToDMPSize<type>(); }

    bool Pack(uint8_t *data, unsigned int &length) const {
      if (length < Size()) {
        length = 0;
        return false;
      }
      type field[3];
      field[0] = HostToNetwork(m_start);
      field[1] = HostToNetwork(m_increment);
      field[2] = HostToNetwork(m_number);
      memcpy(data, &field, Size());
      length = Size();
      return true;
    }

    void Write(ola::io::OutputStream *stream) const {
      type field[3];
      field[0] = HostToNetwork(m_start);
      field[1] = HostToNetwork(m_increment);
      field[2] = HostToNetwork(m_number);
      stream->Write(reinterpret_cast<uint8_t*>(&field), Size());
    }

    bool IsRange() const { return true; }

  protected:
    unsigned int BaseSize() const { return sizeof(type); }

  private:
    type m_start, m_increment, m_number;
};


typedef RangeDMPAddress<uint8_t> OneByteRangeDMPAddress;
typedef RangeDMPAddress<uint16_t> TwoByteRangeDMPAddress;
typedef RangeDMPAddress<uint32_t> FourByteRangeDMPAddress;


/*
 * Create a new range address.
 */
const BaseDMPAddress *NewRangeAddress(unsigned int value,
                                      unsigned int increment,
                                      unsigned int number);

/*
 * Decode an Address
 */
const BaseDMPAddress *DecodeAddress(dmp_address_size size,
                                    dmp_address_type type,
                                    const uint8_t *data,
                                    unsigned int &length);


/*
 * A DMPAddressData object, this hold an address/data pair
 * @param type either DMPAddress<> or RangeDMPAddress<>
 */
template <typename type>
class DMPAddressData {
  public:
    DMPAddressData(const type *address,
                   const uint8_t *data,
                   unsigned int length):
      m_address(address),
      m_data(data),
      m_length(length) {}

    const type *Address() const { return m_address; }
    const uint8_t *Data() const { return m_data; }
    unsigned int Size() const { return m_address->Size() + m_length; }

    // Pack the data into a buffer
    bool Pack(uint8_t *data, unsigned int &length) const {
      if (!m_data)
        return false;

      unsigned int total = length;
      if (!m_address->Pack(data, length)) {
        length = 0;
        return false;
      }
      if (total - length < m_length) {
        length = 0;
        return false;
      }
      memcpy(data + length, m_data, m_length);
      length += m_length;
      return true;
    }

    void Write(ola::io::OutputStream *stream) const {
      if (!m_data)
        return;

      m_address->Write(stream);
      stream->Write(m_data, m_length);
    }

  private:
    const type *m_address;
    const uint8_t *m_data;
    unsigned int m_length;
};
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131_DMPADDRESS_H_
