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
 * DMPPDU.h
 * Interface for the DMP PDU
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef OLA_DMP_DMPPDU_H
#define OLA_DMP_DMPPDU_H

#include <stdint.h>
#include <vector>

#include "DMPAddress.h"
#include "DMPHeader.h"
#include "PDU.h"

namespace ola {
namespace e131 {

using std::vector;


static const unsigned int MAX_TWO_BYTE = 0xffff;
static const unsigned int MAX_ONE_BYTE = 0xff;

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
    bool PackHeader(uint8_t *data, unsigned int &length) const;

  protected:
    DMPHeader m_header;
};


/*
 * A DMPGetPropertyPDU, templatized by the address type. This handles both
 * single and repeated addresses. In the future it may be worthwhile to split
 * the classes for better performance (saves a vector copy in the single case).
 * Don't create these directly, instead use the helper function below which
 * enforce compile time consistency.
 */
template <typename Address>
class DMPGetProperty: public DMPPDU {
  public:
    static const unsigned int VECTOR = 1;

    DMPGetProperty(const DMPHeader &header,
                   const vector<Address> &addresses):
      DMPPDU(VECTOR, header),
      m_addresses(addresses) {}

    unsigned int DataSize() const {
      return m_addresses.size() * m_header.Size();
    }

    bool PackData(uint8_t *data, unsigned int &length) const {
      typename vector<Address>::const_iterator iter;
      unsigned int offset = 0;
      for (iter = m_addresses.begin(); iter != m_addresses.end(); ++iter) {
        unsigned int remaining = length - offset;
        if (!iter->Pack(data + offset, remaining))
          return false;
        offset += remaining;
      }
      return true;
    }

  private:
    vector<Address> m_addresses;
};


template <typename type>
const DMPPDU *_CreateSingleDMPGetProperty(bool is_virtual,
                                          bool is_relative,
                                          unsigned int start) {
  SingleDMPAddress<type> address(start);
  vector<SingleDMPAddress<type> > addresses;
  addresses.push_back(address);
  DMPHeader header(is_virtual,
                   is_relative,
                   DMPHeader::NON_RANGE,
                   DMPTypeToSize<type>());
  return new DMPGetProperty<SingleDMPAddress<type> >(header, addresses);
}


/*
 * Create a new Single Address GetProperty PDU.
 * @param is_virtual set to true if this is a virtual address
 * @param is_relative set to true if this is a relative address
 * @param start the start offset
 * @return A pointer to a DMPGetProperty.
 */
const DMPPDU *SingleDMPGetProperty(bool is_virtual,
                                   bool is_relative,
                                   unsigned int start) {
  if (start > MAX_TWO_BYTE)
    return _CreateSingleDMPGetProperty<uint32_t>(is_virtual,
                                                 is_relative,
                                                 start);
  else if (start > MAX_ONE_BYTE)
    return _CreateSingleDMPGetProperty<uint16_t>(is_virtual,
                                                 is_relative,
                                                 start);
  return _CreateSingleDMPGetProperty<uint8_t>(is_virtual, is_relative, start);
}


/*
 * Create a Repeated DMP GetProperty Message.
 * @param type uint8_t, uint16_t or uint32_t
 * @param is_virtual set to true if this is a virtual address
 * @param is_relative set to true if this is a relative address
 * @param addresses a vector of addresses that match the type
 * @return A pointer to a DMPGetProperty.
 */
template <typename type>
const DMPPDU *DMPRepeatedGetProperty(
    bool is_virtual,
    bool is_relative,
    const vector<RepeatedDMPAddress<type> > &addresses) {
  DMPHeader header(is_virtual,
                   is_relative,
                   DMPHeader::RANGE_SINGLE,
                   DMPTypeToSize<type>());
  return new DMPGetProperty<RepeatedDMPAddress<type> >(header, addresses);
}


template <typename type>
const DMPPDU *_CreateDMPRepeatedGetProperty(bool is_virtual,
                                            bool is_relative,
                                            unsigned int start,
                                            unsigned int increment,
                                            unsigned int number) {
  vector<RepeatedDMPAddress<type> > addresses;
  RepeatedDMPAddress<type> address(start, increment, number);
  addresses.push_back(address);
  return DMPRepeatedGetProperty<type>(is_virtual,
                                      is_relative,
                                      addresses);
}


/*
 * Create a new repeated address GetProperty PDU.
 * @param is_virtual set to true if this is a virtual address
 * @param is_relative set to true if this is a relative address
 * @param start the start offset
 * @param increment the increments between addresses
 * @param number the number of addresses defined
 * @return A pointer to a DMPGetProperty.
 */
const DMPPDU *RepeatedDMPGetProperty(
    bool is_virtual,
    bool is_relative,
    unsigned int start,
    unsigned int increment,
    unsigned int number) {

  if (start > MAX_TWO_BYTE || increment > MAX_TWO_BYTE ||
      number > MAX_TWO_BYTE)
    return _CreateDMPRepeatedGetProperty<uint32_t>(is_virtual,
                                                   is_relative,
                                                   start,
                                                   increment,
                                                   number);
  else if (start > MAX_ONE_BYTE || increment > MAX_ONE_BYTE ||
             number > MAX_ONE_BYTE)
    return _CreateDMPRepeatedGetProperty<uint16_t>(is_virtual,
                                                   is_relative,
                                                   start,
                                                   increment,
                                                   number);
  return _CreateDMPRepeatedGetProperty<uint8_t>(is_virtual,
                                                is_relative,
                                                start,
                                                increment,
                                                number);
}


} // e131
} // ola

#endif
