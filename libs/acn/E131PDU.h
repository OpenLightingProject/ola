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
 * E131PDU.h
 * Interface for the E131PDU class
 * Copyright (C) 2007 Simon Newton
 */

#ifndef LIBS_ACN_E131PDU_H_
#define LIBS_ACN_E131PDU_H_

#include "libs/acn/PDU.h"
#include "libs/acn/E131Header.h"

namespace ola {
namespace acn {

class DMPPDU;

class E131PDU: public PDU {
 public:
  E131PDU(unsigned int vector,
          const E131Header &header,
          const DMPPDU *dmp_pdu):
    PDU(vector),
    m_header(header),
    m_dmp_pdu(dmp_pdu),
    m_data(NULL),
    m_data_size(0) {}

  E131PDU(unsigned int vector,
          const E131Header &header,
          const uint8_t *data,
          unsigned int data_size):
    PDU(vector),
    m_header(header),
    m_dmp_pdu(NULL),
    m_data(data),
    m_data_size(data_size) {}

  ~E131PDU() {}

  unsigned int HeaderSize() const;
  unsigned int DataSize() const;
  bool PackHeader(uint8_t *data, unsigned int *length) const;
  bool PackData(uint8_t *data, unsigned int *length) const;

  void PackHeader(ola::io::OutputStream *stream) const;
  void PackData(ola::io::OutputStream *stream) const;

 private:
  E131Header m_header;
  const DMPPDU *m_dmp_pdu;
  const uint8_t *m_data;
  const unsigned int m_data_size;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_E131PDU_H_
