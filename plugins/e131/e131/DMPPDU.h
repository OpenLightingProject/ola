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

#include "PDU.h"
#include "DMPHeader.h"

namespace ola {
namespace e131 {

class DmpMsg;

class DMPPDU: public PDU {
  public:
    DMPPDU(unsigned int vector, const DMPHeader &header, const DmpMsg *msg):
      PDU(vector),
      m_header(header),
      m_dmp(msg) {}
    ~DMPPDU() {}

    unsigned int HeaderSize() const { return DMPHeader::DMP_HEADER_SIZE; }
    unsigned int DataSize() const;
    bool PackHeader(uint8_t *data, unsigned int &length) const;
    bool PackData(uint8_t *data, unsigned int &length) const;

  private:
    DMPHeader m_header;
    const DmpMsg *m_dmp;

};

} // e131
} // ola

#endif
