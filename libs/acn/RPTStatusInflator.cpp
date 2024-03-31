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
 * RPTStatusInflator.cpp
 * The Inflator for the RPT Status PDUs
 * Copyright (C) 2024 Peter Newman
 */

#include <memory>
#include <string>
#include "ola/Logging.h"
#include "ola/e133/E133StatusHelper.h"
#include "libs/acn/RPTStatusInflator.h"

namespace ola {
namespace acn {

using std::string;

/**
 * Create a new RPT Status inflator
 */
RPTStatusInflator::RPTStatusInflator()
    : BaseInflator(PDU::TWO_BYTES) {
}

/**
 * Set an RPTStatusHandler to run when receiving an RPT Status message.
 * @param handler the callback to invoke when there is an RPT status message.
 */
void RPTStatusInflator::SetRPTStatusHandler(RPTStatusHandler *handler) {
  m_rpt_status_handler.reset(handler);
}


/*
 * Decode the RPT Status 'header', which is 0 bytes in length.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool RPTStatusInflator::DecodeHeader(HeaderSet *,
                                     const uint8_t*,
                                     unsigned int,
                                     unsigned int *bytes_used) {
  *bytes_used = 0;
  return true;
}


/*
 * Handle an RPT Status PDU for E1.33.
 */
bool RPTStatusInflator::HandlePDUData(uint32_t vector,
                                      OLA_UNUSED const HeaderSet &headers,
                                      OLA_UNUSED const uint8_t *data,
                                      OLA_UNUSED unsigned int pdu_len) {
  ola::acn::RPTStatusVector status_vector;
  if (!ola::e133::IntToRPTStatusCode(static_cast<uint16_t>(vector),
                                     &status_vector)) {
    OLA_WARN << "Unknown RPT status message vector was " << vector;
    return true;
  } else {
    OLA_INFO << "RPT status message vector was "
             << ola::e133::RPTStatusCodeToString(status_vector);
  }
  // TODO(Peter): Handle the optional string message for the vectors that can
  // have it/alert for those that can't if they do

  if (m_rpt_status_handler.get()) {
    E133Header e133_header = headers.GetE133Header();

    m_rpt_status_handler->Run(&headers.GetTransportHeader(), &e133_header,
                              status_vector, "");
  } else {
    OLA_WARN << "No RPT Status handler defined!";
  }
  return true;
}
}  // namespace acn
}  // namespace ola
