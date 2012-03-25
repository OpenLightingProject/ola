/*
#include <ola/Logging.h>
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
 * PreamblePacker.h
 * This class takes a block of Root Layer PDUs, prepends the ACN preamble and
 * writes the data to a buffer.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef PLUGINS_E131_E131_PREAMBLEPACKER_H_
#define PLUGINS_E131_E131_PREAMBLEPACKER_H_

#include "plugins/e131/e131/PDU.h"

namespace ola {
namespace plugin {
namespace e131 {


/*
 * Pack a Root PDU block and the ACN Preamble into a memory block. This class
 * isn't reentrant so be careful where you use it.
 */
class PreamblePacker {
  public:
    explicit PreamblePacker()
        : m_send_buffer(NULL) {
    }
    ~PreamblePacker();

    const uint8_t *Pack(const PDUBlock<PDU> &pdu_block,
                        unsigned int *length);

    static const char ACN_PACKET_ID[];  // ASC-E1.17\0\0\0
    static const unsigned int MAX_DATAGRAM_SIZE = 1472;
    static const uint16_t PREAMBLE_SIZE = 0x10;
    static const uint16_t POSTABLE_SIZE = 0;
    static const unsigned int PREAMBLE_OFFSET = 4;
    static const unsigned int DATA_OFFSET = PREAMBLE_OFFSET + 12;

  private:
    uint8_t *m_send_buffer;

    void Init();
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_PREAMBLEPACKER_H_
