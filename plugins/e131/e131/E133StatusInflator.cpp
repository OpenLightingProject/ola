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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * E133StatusInflator.cpp
 * The Inflator for the E1.33 Status messages.
 * Copyright (C) 2013 Simon Newton
 */

#include <string>
#include <algorithm>
#include "ola/e133/E133Enums.h"
#include "plugins/e131/e131/E133StatusInflator.h"

namespace ola {
namespace plugin {
namespace e131 {

using std::string;

/**
 * Create a new E1.33 status inflator
 */
E133StatusInflator::E133StatusInflator()
    : BaseInflator(PDU::TWO_BYTES) {
}


/*
 * Handle a E1.33 Status PDU.
 */
bool E133StatusInflator::HandlePDUData(uint32_t vector,
                                       const HeaderSet &headers,
                                       const uint8_t *data,
                                       unsigned int pdu_len) {
  unsigned int size = std::min(
      pdu_len,
      static_cast<unsigned int>(ola::e133::MAX_E133_STATUS_STRING_SIZE));
  string description(reinterpret_cast<const char*>(&data[0]), size);

  m_handler->Run(&headers.GetTransportHeader(),
                 &headers.GetE133Header(),
                 static_cast<uint16_t>(vector),
                 description);
  return true;
}
}  // namespace e131
}  // namespace plugin
}  // namespace ola
