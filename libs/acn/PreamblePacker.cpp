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
 * PreamblePacker.cpp
 * The PreamblePacker class
 * Copyright (C) 2007 Simon Newton
 */

#include <string.h>
#include <string>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/io/BigEndianStream.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "libs/acn/BaseInflator.h"
#include "libs/acn/HeaderSet.h"
#include "libs/acn/PreamblePacker.h"

namespace ola {
namespace acn {

using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::io::IOStack;

const uint8_t PreamblePacker::ACN_HEADER[] = {
  0x00, 0x10,
  0x00, 0x00,
  0x41, 0x53, 0x43, 0x2d,
  0x45, 0x31, 0x2e, 0x31,
  0x37, 0x00, 0x00, 0x00
};
const unsigned int PreamblePacker::ACN_HEADER_SIZE = sizeof(ACN_HEADER);

const uint8_t PreamblePacker::TCP_ACN_HEADER[] = {
  // No pre or post amble size for TCP
  0x41, 0x53, 0x43, 0x2d,
  0x45, 0x31, 0x2e, 0x31,
  0x37, 0x00, 0x00, 0x00
  // For TCP, the next 4 bytes are the block size
};
const unsigned int PreamblePacker::TCP_ACN_HEADER_SIZE = sizeof(TCP_ACN_HEADER);

/*
 * Clean up
 */
PreamblePacker::~PreamblePacker() {
  if (m_send_buffer)
    delete[] m_send_buffer;
}


/*
 * Pack the PDU block along with the preamble into a memory location.
 * @param pdu_block the block of pdus to send
 * @param length the size of the data buffer to send.
 * @return
 */
const uint8_t *PreamblePacker::Pack(const PDUBlock<PDU> &pdu_block,
                                    unsigned int *length) {
  if (!m_send_buffer)
    Init();

  unsigned int size = MAX_DATAGRAM_SIZE -
      static_cast<unsigned int>(sizeof(ACN_HEADER));
  if (!pdu_block.Pack(m_send_buffer + sizeof(ACN_HEADER), &size)) {
    OLA_WARN << "Failed to pack E1.31 PDU";
    return NULL;
  }
  *length = static_cast<unsigned int>(sizeof(ACN_HEADER) + size);
  return m_send_buffer;
}


/**
 * Add the UDP Preamble to an IOStack
 */
void PreamblePacker::AddUDPPreamble(IOStack *stack) {
  ola::io::BigEndianOutputStream output(stack);
  stack->Write(ACN_HEADER, ACN_HEADER_SIZE);
}


/**
 * Add the TCP Preamble to an IOStack
 */
void PreamblePacker::AddTCPPreamble(IOStack *stack) {
  ola::io::BigEndianOutputStream output(stack);
  output << stack->Size();
  stack->Write(TCP_ACN_HEADER, TCP_ACN_HEADER_SIZE);
}


/*
 * Allocate memory for the data.
 */
void PreamblePacker::Init() {
  if (!m_send_buffer) {
    m_send_buffer = new uint8_t[MAX_DATAGRAM_SIZE];
    memset(m_send_buffer + sizeof(ACN_HEADER),
           0,
           MAX_DATAGRAM_SIZE - sizeof(ACN_HEADER));
    memcpy(m_send_buffer, ACN_HEADER, sizeof(ACN_HEADER));
  }
}
}  // namespace acn
}  // namespace ola
