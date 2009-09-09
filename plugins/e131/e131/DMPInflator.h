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
 * DMPInflator.h
 * Interface for the DMPInflator class.
 * Copyright (C) 2009 Simon Newton
 */

#ifndef OLA_DMP_DMPINFLATOR_H
#define OLA_DMP_DMPINFLATOR_H

#include "BaseInflator.h"
#include "DMPHeader.h"

namespace ola {
namespace e131 {

class DMPInflator: public BaseInflator {
  friend class DMPInflatorTest;

  public:
    DMPInflator(): BaseInflator(PDU::ONE_BYTE),
                   m_last_header_valid(false) {
    }
    virtual ~DMPInflator() {}

    unsigned int static const DMP_VECTOR = 2;
    uint32_t Id() const { return DMP_VECTOR; }

  protected:
    bool DecodeHeader(HeaderSet &headers, const uint8_t *data,
                      unsigned int len, unsigned int &bytes_used);

    void ResetHeaderField();
  private:
    DMPHeader m_last_header;
    bool m_last_header_valid;
};

} // e131
} // ola

#endif
