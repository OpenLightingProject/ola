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
 * PreamblePacker.h
 * The PreamblePacker class
 * Copyright (C) 2007 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <string.h>
#include <string>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/BaseInflator.h"
#include "plugins/e131/e131/HeaderSet.h"
#include "plugins/e131/e131/PreamblePacker.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::HostToNetwork;
using ola::network::IPV4Address;

const char PreamblePacker::ACN_PACKET_ID[] = "ASC-E1.17\0\0\0";

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

  unsigned int size = MAX_DATAGRAM_SIZE - DATA_OFFSET;
  if (!pdu_block.Pack(m_send_buffer + DATA_OFFSET, size)) {
    OLA_WARN << "Failed to pack E1.31 PDU";
    return NULL;
  }
  *length = DATA_OFFSET + size;
  return m_send_buffer;
}


/*
 * Allocate memory for the data.
 */
void PreamblePacker::Init() {
  if (!m_send_buffer) {
    m_send_buffer = new uint8_t[MAX_DATAGRAM_SIZE];
    memset(m_send_buffer, 0, DATA_OFFSET);
    uint16_t *ptr = reinterpret_cast<uint16_t*>(m_send_buffer);
    *ptr++ = HostToNetwork(PREAMBLE_SIZE);
    *ptr = HostToNetwork(POSTABLE_SIZE);
    strncpy(reinterpret_cast<char*>(m_send_buffer + PREAMBLE_OFFSET),
            ACN_PACKET_ID,
            strlen(ACN_PACKET_ID));
  }
}
}  // e131
}  // plugin
}  // ola
