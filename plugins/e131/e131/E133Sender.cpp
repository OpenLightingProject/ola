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
 * E133Sender.cpp
 * The E133Sender
 * Copyright (C) 2011 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/DMPE133Inflator.h"
#include "plugins/e131/e131/DMPPDU.h"
#include "plugins/e131/e131/E133Inflator.h"
#include "plugins/e131/e131/E133Sender.h"
#include "plugins/e131/e131/E133PDU.h"
#include "plugins/e131/e131/RootSender.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::HostToNetwork;

/*
 * Create a new E133Sender
 * @param root_sender the root layer to use
 */
E133Sender::E133Sender(RootSender *root_sender)
    : m_root_sender(root_sender) {
  if (!m_root_sender)
    OLA_WARN << "root_sender is null, this won't work";
}


/*
 * Send a DMPPDU
 * @param header the E133Header
 * @param dmp_pdu the DMPPDU to send
 * @param transport the OutgoingTransport to use when sending the message.
 */
bool E133Sender::SendDMP(const E133Header &header,
                        const DMPPDU *dmp_pdu,
                        OutgoingTransport *transport) {
  if (!m_root_sender)
    return false;

  E133PDU pdu(DMPInflator::DMP_VECTOR, header, dmp_pdu);
  unsigned int vector = E133Inflator::E133_VECTOR;
  return m_root_sender->SendPDU(vector, pdu, transport);
}
}  // e131
}  // plugin
}  // ola
