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
 * RootPDU.h
 * Interface for the RootPDU class
 * Copyright (C) 2007 Simon Newton
 */

#ifndef LIBS_ACN_ROOTPDU_H_
#define LIBS_ACN_ROOTPDU_H_

#include <stdint.h>

#include "ola/acn/CID.h"
#include "ola/io/IOStack.h"

#include "libs/acn/PDU.h"

namespace ola {
namespace acn {

class RootPDU: public PDU {
 public:
  explicit RootPDU(unsigned int vector, bool force_length_flag = false):
    PDU(vector, FOUR_BYTES, force_length_flag),
    m_block(NULL),
    m_block_size(0) {}
  RootPDU(unsigned int vector,
          const ola::acn::CID &cid,
          const PDUBlock<PDU> *block):
    PDU(vector),
    m_cid(cid),
    m_block(block) {
    m_block_size = block ? block->Size() : 0;
  }
  ~RootPDU() {}

  unsigned int HeaderSize() const { return ola::acn::CID::CID_LENGTH; }
  unsigned int DataSize() const { return m_block_size; }
  bool PackHeader(uint8_t *data, unsigned int *length) const;
  bool PackData(uint8_t *data, unsigned int *length) const;

  void PackHeader(ola::io::OutputStream *stream) const;
  void PackData(ola::io::OutputStream *stream) const;

  const ola::acn::CID &Cid() const { return m_cid; }
  const ola::acn::CID &Cid(const ola::acn::CID &cid) { return m_cid = cid; }
  void SetBlock(const PDUBlock<PDU> *block);

  static void PrependPDU(ola::io::IOStack *stack, uint32_t vector,
                         const ola::acn::CID &cid, bool force_length_flag = false);

 private:
  ola::acn::CID m_cid;
  const PDUBlock<PDU> *m_block;
  unsigned int m_block_size;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_ROOTPDU_H_
