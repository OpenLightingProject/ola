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

#include <algorithm>
#include <map>
#include "ola/Logging.h"
#include "plugins/e131/e131/DMPE131Inflator.h"
#include "plugins/e131/e131/DMPHeader.h"
#include "plugins/e131/e131/DMPPDU.h"

namespace ola {
namespace plugin {
namespace e131 {

using std::map;
using ola::Closure;


DMPE131Inflator::~DMPE131Inflator() {
  map<unsigned int, universe_handler>::iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter) {
    delete iter->second.closure;
    m_e131_layer->LeaveUniverse(iter->first);
  }
  m_handlers.clear();
}


/*
 * Handle a DMP PDU for E1.31.
 */
bool DMPE131Inflator::HandlePDUData(uint32_t vector,
                                    HeaderSet &headers,
                                    const uint8_t *data,
                                    unsigned int pdu_len) {
  if (vector != DMP_SET_PROPERTY_VECTOR) {
    OLA_INFO << "not a set property msg: " << vector;
    return true;
  }

  E131Header e131_header = headers.GetE131Header();
  map<unsigned int, universe_handler>::iterator iter =
      m_handlers.find(e131_header.Universe());

  if (iter == m_handlers.end())
    return true;

  DMPHeader dmp_header = headers.GetDMPHeader();

  if (!dmp_header.IsVirtual() || dmp_header.IsRelative() ||
      dmp_header.Size() != TWO_BYTES ||
      dmp_header.Type() != RANGE_EQUAL) {
    OLA_DEBUG << "malformed E1.31 dmp header " << dmp_header.Header();
    return true;
  }

  unsigned int available_length = pdu_len;
  const BaseDMPAddress *address = DecodeAddress(dmp_header.Size(),
                                                dmp_header.Type(),
                                                data,
                                                available_length);

  if (address->Start()) {
    delete address;
    return true;
  }

  if (address->Increment() != 1) {
    OLA_INFO << "E1.31 DMP packet with increment " << address->Increment()
      << ", disarding";
    return true;
  }

  unsigned int channels = std::min(pdu_len - available_length,
                                   address->Number());
  iter->second.buffer->Set(data + available_length, channels);
  iter->second.closure->Run();
  delete address;

  return true;
}

/*
 * Set the closure to be called when we receive data for this universe.
 * @param universe the universe to register the handler for
 * @param buffer the DmxBuffer to update with the data
 * @param handler the Closure to call when there is data for this universe.
 * Ownership of the closure is transferred to the node.
 */
bool DMPE131Inflator::SetHandler(unsigned int universe,
                                 ola::DmxBuffer *buffer,
                                 ola::Closure *closure) {
  if (!closure || !buffer)
    return false;

  map<unsigned int, universe_handler>::iterator iter =
    m_handlers.find(universe);

  if (iter == m_handlers.end()) {
    universe_handler handler;
    handler.buffer = buffer;
    handler.closure = closure;
    m_handlers[universe] = handler;
    m_e131_layer->JoinUniverse(universe);
  } else {
    Closure *old_closure = iter->second.closure;
    iter->second.closure = closure;
    iter->second.buffer = buffer;
    delete old_closure;
  }
  return true;
}


/*
 * Remove the handler for this universe
 * @param universe the universe handler to remove
 * @param true if removed, false if it didn't exist
 */
bool DMPE131Inflator::RemoveHandler(unsigned int universe) {
  map<unsigned int, universe_handler>::iterator iter =
    m_handlers.find(universe);

  if (iter != m_handlers.end()) {
    Closure *old_closure = iter->second.closure;
    m_handlers.erase(iter);
    m_e131_layer->LeaveUniverse(universe);
    delete old_closure;
    return true;
  }
  return false;
}
}  // e131
}  // plugin
}  // ola
