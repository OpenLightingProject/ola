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
 * DMPE131Inflator.cpp
 * The Inflator for the DMP PDUs
 * Copyright (C) 2007-2009 Simon Newton
 */

#include <ola/Logging.h>
#include "DMPE131Inflator.h"

namespace ola {
namespace e131 {


/*
 * Handle a DMP PDU.
 */
bool DMPE131Inflator::HandlePDUData(uint32_t vector,
                                HeaderSet &headers,
                                const uint8_t *data,
                                unsigned int pdu_len) {

  E131Header e131_header = headers.GetE131Header();
  unsigned int seq = e131_header.Sequence();
  unsigned int pri = e131_header.Priority();
  OLA_INFO << "in DMP handler, uni " << e131_header.Universe() << ", seq " <<
    seq << ", pri " << pri << ".";
  return true;
}

} // e131
} // ola
