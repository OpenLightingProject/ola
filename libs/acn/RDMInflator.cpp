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
 * RDMInflator.cpp
 * The Inflator for the RDM PDUs
 * Copyright (C) 2011 Simon Newton
 */

#include <memory>
#include <string>
#include "ola/Logging.h"
#include "ola/rdm/RDMCommand.h"
#include "libs/acn/RDMInflator.h"

namespace ola {
namespace acn {

using std::string;

/**
 * Create a new RDM inflator
 */
RDMInflator::RDMInflator(unsigned int vector)
    : BaseInflator(PDU::ONE_BYTE),
      m_vector(vector) {
}

/**
 * Set an RDMMessageHandler to run when receiving a RDM message.
 * @param handler the callback to invoke when there is RDM data for this
 * universe.
 */
void RDMInflator::SetRDMHandler(RDMMessageHandler *handler) {
  m_rdm_handler.reset(handler);
}


/**
 * Set a GenericRDMHandler to run when receiving a RDM message.
 * @param handler the callback to invoke when there is RDM data for this
 * universe.
 */
void RDMInflator::SetGenericRDMHandler(GenericRDMMessageHandler *handler) {
  m_generic_rdm_handler.reset(handler);
}


/*
 * Decode the RDM 'header', which is 0 bytes in length.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool RDMInflator::DecodeHeader(HeaderSet *,
                               const uint8_t*,
                               unsigned int,
                               unsigned int *bytes_used) {
  *bytes_used = 0;
  return true;
}


/*
 * Handle an RDM PDU for E1.33.
 */
bool RDMInflator::HandlePDUData(uint32_t vector,
                                const HeaderSet &headers,
                                const uint8_t *data,
                                unsigned int pdu_len) {
  if (vector != VECTOR_RDM_CMD_RDM_DATA) {
    OLA_INFO << "Not a RDM message, vector was " << vector;
    return true;
  }

  string rdm_message(reinterpret_cast<const char*>(&data[0]), pdu_len);

  if (m_rdm_handler.get()) {
    E133Header e133_header = headers.GetE133Header();

    m_rdm_handler->Run(&headers.GetTransportHeader(), &e133_header,
                       rdm_message);
  } else if (m_generic_rdm_handler.get()) {
    m_generic_rdm_handler->Run(&headers, rdm_message);
  } else {
    OLA_WARN << "No RDM handler defined!";
  }
  return true;
}
}  // namespace acn
}  // namespace ola
