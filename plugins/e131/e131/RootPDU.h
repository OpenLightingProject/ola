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
 * RootPDU.h
 * Interface for the RootPDU class
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef OLA_E131_ROOTPDU_H
#define OLA_E131_ROOTPDU_H

#include <stdint.h>

#include "CID.h"
#include "PDU.h"


namespace ola {
namespace e131 {

class RootPDU: public PDU {
  public:
    RootPDU():
      PDU(),
      m_vector(0),
      m_block(NULL) {}
    RootPDU(unsigned int vector, const CID &cid, PDUBlock<PDU> *block):
      PDU(),
      m_vector(vector),
      m_cid(cid),
      m_block(block) {}
    ~RootPDU() {}

    unsigned int Size() const;
    bool Pack(uint8_t *data, unsigned int &length) const;

    unsigned int Vector() const { return m_vector; }
    unsigned int Vector(unsigned int vector) { return m_vector = vector; }

    void SetBlock(PDUBlock<PDU> *block) { m_block = block; }

    const CID &Cid() const { return m_cid; }
    const CID &Cid(CID &cid) { return m_cid = cid; }

  private:
    uint32_t m_vector;
    CID m_cid;
    PDUBlock<PDU> *m_block;
};

} // e131
} // ola

#endif
