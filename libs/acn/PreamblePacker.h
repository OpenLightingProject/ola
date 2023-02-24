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
 * PreamblePacker.h
 * This class takes a block of Root Layer PDUs, prepends the ACN preamble and
 * writes the data to a buffer.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef LIBS_ACN_PREAMBLEPACKER_H_
#define LIBS_ACN_PREAMBLEPACKER_H_

#include "ola/io/IOStack.h"
#include "libs/acn/PDU.h"

namespace ola {
namespace acn {

/*
 * Pack a Root PDU block and the ACN Preamble into a memory block. This class
 * isn't reentrant so be careful where you use it.
 */
class PreamblePacker {
 public:
    PreamblePacker() : m_send_buffer(NULL) {}
    ~PreamblePacker();

    const uint8_t *Pack(const PDUBlock<PDU> &pdu_block,
                        unsigned int *length);

    static void AddUDPPreamble(ola::io::IOStack *stack);
    static void AddTCPPreamble(ola::io::IOStack *stack);

    static const uint8_t ACN_HEADER[];
    static const unsigned int ACN_HEADER_SIZE;
    static const unsigned int MAX_DATAGRAM_SIZE = 1472;

 private:
    uint8_t *m_send_buffer;

    void Init();

    static const uint8_t TCP_ACN_HEADER[];
    static const unsigned int TCP_ACN_HEADER_SIZE;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_PREAMBLEPACKER_H_
