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
 * DMPPDU.h
 * Interface for the DMP PDU
 * Copyright (C) 2007 Simon Newton
 */

#ifndef LIBS_ACN_DMPPDU_H_
#define LIBS_ACN_DMPPDU_H_

#include <stdint.h>
#include <vector>

#include "ola/acn/ACNVectors.h"
#include "libs/acn/DMPAddress.h"
#include "libs/acn/DMPHeader.h"
#include "libs/acn/PDU.h"

namespace ola {
namespace acn {

/*
 * The base DMPPDU class.
 * More specific dmp pdus like the SetPropery inherit from this.
 */
class DMPPDU: public PDU {
 public:
    DMPPDU(unsigned int vector, const DMPHeader &dmp_header):
      PDU(vector, ONE_BYTE),
      m_header(dmp_header) {
    }
    ~DMPPDU() {}

    unsigned int HeaderSize() const { return DMPHeader::DMP_HEADER_SIZE; }
    bool PackHeader(uint8_t *data, unsigned int *length) const;
    void PackHeader(ola::io::OutputStream *stream) const;

 protected:
    DMPHeader m_header;
};


/*
 * A DMPGetPropertyPDU, templatized by the address type.
 * Don't create these directly, instead use the helper function below which
 * enforces compile time consistency.
 */
template <typename Address>
class DMPGetProperty: public DMPPDU {
 public:
    DMPGetProperty(const DMPHeader &header,
                   const std::vector<Address> &addresses):
      DMPPDU(ola::acn::DMP_GET_PROPERTY_VECTOR, header),
      m_addresses(addresses) {}

    unsigned int DataSize() const {
      return static_cast<unsigned int>(m_addresses.size() * m_header.Bytes() *
              (m_header.Type() == NON_RANGE ? 1 : 3));
    }

    bool PackData(uint8_t *data, unsigned int *length) const {
      typename std::vector<Address>::const_iterator iter;
      unsigned int offset = 0;
      for (iter = m_addresses.begin(); iter != m_addresses.end(); ++iter) {
        unsigned int remaining = *length - offset;
        if (!iter->Pack(data + offset, &remaining))
          return false;
        offset += remaining;
      }
      *length = offset;
      return true;
    }

    void PackData(ola::io::OutputStream *stream) const {
      typename std::vector<Address>::const_iterator iter;
      for (iter = m_addresses.begin(); iter != m_addresses.end(); ++iter)
        iter->Write(stream);
    }

 private:
    std::vector<Address> m_addresses;
};


/*
 * Create a non-ranged GetProperty PDU
 * @param type uint8_t, uint16_t or uint32_t
 * @param is_virtual set to true if this is a virtual address
 * @param is_relative set to true if this is a relative address
 * @param addresses a vector of DMPAddress objects
 */
template <typename type>
const DMPPDU *NewDMPGetProperty(
    bool is_virtual,
    bool is_relative,
    const std::vector<DMPAddress<type> > &addresses) {
  DMPHeader header(is_virtual,
                   is_relative,
                   NON_RANGE,
                   TypeToDMPSize<type>());
  return new DMPGetProperty<DMPAddress<type> >(header, addresses);
}


/*
 * Create a non-ranged DMP GetProperty PDU
 * @param type uint8_t, uint16_t, uint32_t
 */
template <typename type>
const DMPPDU *_CreateDMPGetProperty(bool is_virtual,
                                    bool is_relative,
                                    unsigned int start) {
  DMPAddress<type> address((type) start);
  std::vector<DMPAddress<type> > addresses;
  addresses.push_back(address);
  return NewDMPGetProperty<type>(is_virtual, is_relative, addresses);
}


/*
 * A helper to create a new single, non-ranged GetProperty PDU.
 * @param is_virtual set to true if this is a virtual address
 * @param is_relative set to true if this is a relative address
 * @param start the start offset
 * @return A pointer to a DMPPDU.
 */
const DMPPDU *NewDMPGetProperty(bool is_virtual,
                                bool is_relative,
                                unsigned int start);


/*
 * Create a Ranged DMP GetProperty Message.
 * @param type uint8_t, uint16_t or uint32_t
 * @param is_virtual set to true if this is a virtual address
 * @param is_relative set to true if this is a relative address
 * @param addresses a vector of addresses that match the type
 * @return A pointer to a DMPPDU.
 */
template <typename type>
const DMPPDU *NewRangeDMPGetProperty(
    bool is_virtual,
    bool is_relative,
    const std::vector<RangeDMPAddress<type> > &addresses) {
  DMPHeader header(is_virtual,
                   is_relative,
                   RANGE_SINGLE,
                   TypeToDMPSize<type>());
  return new DMPGetProperty<RangeDMPAddress<type> >(header, addresses);
}


template <typename type>
const DMPPDU *_CreateRangeDMPGetProperty(bool is_virtual,
                                         bool is_relative,
                                         unsigned int start,
                                         unsigned int increment,
                                         unsigned int number) {
  std::vector<RangeDMPAddress<type> > addresses;
  RangeDMPAddress<type> address((type) start, (type) increment, (type) number);
  addresses.push_back(address);
  return NewRangeDMPGetProperty<type>(is_virtual, is_relative, addresses);
}


/*
 * A helper to create a new ranged address GetProperty PDU.
 * @param is_virtual set to true if this is a virtual address
 * @param is_relative set to true if this is a relative address
 * @param start the start offset
 * @param increment the increments between addresses
 * @param number the number of addresses defined
 * @return A pointer to a DMPGetProperty.
 */
const DMPPDU *NewRangeDMPGetProperty(
    bool is_virtual,
    bool is_relative,
    unsigned int start,
    unsigned int increment,
    unsigned int number);


/*
 * A DMPSetPropertyPDU, templatized by the address type.
 * Don't create these directly, instead use the helper functions below which
 * enforce compile time consistency.
 * @param type either DMPAddress<> or RangeDMPAddress<>
 */
template <typename type>
class DMPSetProperty: public DMPPDU {
 public:
    typedef std::vector<DMPAddressData<type> > AddressDataChunks;

    DMPSetProperty(const DMPHeader &header, const AddressDataChunks &chunks):
      DMPPDU(ola::acn::DMP_SET_PROPERTY_VECTOR, header),
      m_chunks(chunks) {}

    unsigned int DataSize() const {
      typename AddressDataChunks::const_iterator iter;
      unsigned int length = 0;
      for (iter = m_chunks.begin(); iter != m_chunks.end(); ++iter)
        length += iter->Size();
      return length;
    }

    bool PackData(uint8_t *data, unsigned int *length) const {
      typename AddressDataChunks::const_iterator iter;
      unsigned int offset = 0;
      for (iter = m_chunks.begin(); iter != m_chunks.end(); ++iter) {
        unsigned int remaining = *length - offset;
        if (!iter->Pack(data + offset, &remaining))
          return false;
        offset += remaining;
      }
      *length = offset;
      return true;
    }

    void PackData(ola::io::OutputStream *stream) const {
      typename AddressDataChunks::const_iterator iter;
      for (iter = m_chunks.begin(); iter != m_chunks.end(); ++iter)
        iter->Write(stream);
    }

 private:
    AddressDataChunks m_chunks;
};


/*
 * Create a new DMP SetProperty Message
 */
template <typename type>
const DMPPDU *NewDMPSetProperty(
    bool is_virtual,
    bool is_relative,
    const std::vector<DMPAddressData<DMPAddress<type> > > &chunks) {

  DMPHeader header(is_virtual,
                   is_relative,
                   NON_RANGE,
                   TypeToDMPSize<type>());
  return new DMPSetProperty<DMPAddress<type> >(header, chunks);
}


/*
 * Create a new DMP SetProperty PDU
 * @param type either DMPAddress or RangeDMPAddress
 * @param is_virtual set to true if this is a virtual address
 * @param is_relative set to true if this is a relative address
 * @param chunks a vector of DMPAddressData<type> objects
 */
template <typename type>
const DMPPDU *NewRangeDMPSetProperty(
    bool is_virtual,
    bool is_relative,
    const std::vector<DMPAddressData<RangeDMPAddress<type> > > &chunks,
    bool multiple_elements = true,
    bool equal_size_elements = true) {

  dmp_address_type address_type;
  if (multiple_elements) {
    if (equal_size_elements)
      address_type = RANGE_EQUAL;
    else
      address_type = RANGE_MIXED;
  } else {
    address_type = RANGE_SINGLE;
  }

  DMPHeader header(is_virtual,
                   is_relative,
                   address_type,
                   TypeToDMPSize<type>());
  return new DMPSetProperty<RangeDMPAddress<type> >(header, chunks);
}
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_DMPPDU_H_
