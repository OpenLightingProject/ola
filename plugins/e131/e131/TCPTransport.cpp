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
 * TCPTransport.cpp
 * The TCPTransport class
 * Copyright (C) 2012 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first

#include "ola/Logging.h"
#include "plugins/e131/e131/TCPTransport.h"

namespace ola {
namespace plugin {
namespace e131 {


/*
 * Send a block of PDU messages.
 * @param pdu_block the block of pdus to send
 */
bool TCPTransport::Send(const PDUBlock<PDU> &pdu_block) {
  unsigned int data_size;
  const uint8_t *data = m_packer->Pack(pdu_block, &data_size);

  if (!data)
    return false;

  // TODO(simon): rather than just attempting to send we should buffer here.
  return m_descriptor->Send(data, data_size);
}
}  // e131
}  // plugin
}  // ola
