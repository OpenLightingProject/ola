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
 * DMPHeader.h
 * The DMP Header
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef OLA_DMP_DMPHEADER_H
#define OLA_DMP_DMPHEADER_H

#include <stdint.h>
#include <string>

namespace ola {
namespace e131 {

using std::string;

/*
 * Header for the DMP layer
 */
class DMPHeader {
  public:
    typedef enum {
      NON_RANGE = 0x00,
      RANGE_SINGLE = 0x01,
      RANGE_EQUAL = 0x02,
      RANGE_MIXED = 0x03,
    } dmp_addr_type;

    typedef enum {
      ONE_OCTET = 0x00,
      TWO_OCTET = 0x01,
      FOUR_OCTET = 0x02,
      RES_OCTET = 0x03
    } dmp_addr_size;

    DMPHeader():
      m_header(0) {}
    DMPHeader(bool is_virtual,
              bool is_relative,
              dmp_addr_type type,
              dmp_addr_size size) {

      m_header = is_virtual << 7 |
               is_relative << 6 |
               type << 4 |
               size;
    }
    ~DMPHeader() {}

    bool IsVirtual() const { return m_header & VIRTUAL_MASK; }
    bool IsRelative() const { return  m_header & RELATIVE_MASK; }

    dmp_addr_type Type() const {
      return (dmp_addr_type) ((m_header & TYPE_MASK) >> 4);
    }

    dmp_addr_size Size() const {
      return (dmp_addr_size) (m_header & SIZE_MASK);
    }

    bool operator==(const DMPHeader &other) const {
      return m_header == other.m_header;
    }

    bool operator!=(const DMPHeader &other) const {
      return m_header != other.m_header;
    }

  private:
    static const uint8_t VIRTUAL_MASK = 0x80;
    static const uint8_t RELATIVE_MASK = 0x40;
    static const uint8_t TYPE_MASK = 0x30;
    static const uint8_t SIZE_MASK = 0x03;
    uint8_t m_header;
};

} // e131
} // ola
#endif
