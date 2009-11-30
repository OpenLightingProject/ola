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

#ifndef PLUGINS_E131_E131_ROOTPDU_H_
#define PLUGINS_E131_E131_ROOTPDU_H_

#include <stdint.h>
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/PDU.h"

namespace ola {
namespace plugin {
namespace e131 {

class RootPDU: public PDU {
  public:
    explicit RootPDU(unsigned int vector):
      PDU(vector),
      m_block(NULL),
      m_block_size(0) {}
    RootPDU(unsigned int vector, const CID &cid, const PDUBlock<PDU> *block):
      PDU(vector),
      m_cid(cid),
      m_block(block) {
      m_block_size = block ? block->Size() : 0;
    }
    ~RootPDU() {}

    unsigned int HeaderSize() const { return CID::CID_LENGTH; }
    unsigned int DataSize() const { return m_block_size; }
    bool PackHeader(uint8_t *data, unsigned int &length) const;
    bool PackData(uint8_t *data, unsigned int &length) const;

    const CID &Cid() const { return m_cid; }
    const CID &Cid(const CID &cid) { return m_cid = cid; }
    void SetBlock(const PDUBlock<PDU> *block);

  private:
    CID m_cid;
    const PDUBlock<PDU> *m_block;
    unsigned int m_block_size;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_ROOTPDU_H_
