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
 * DMPE133Inflator.cpp
 * The Inflator for the DMP PDUs
 * Copyright (C) 2011 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <map>
#include <memory>
#include "ola/Logging.h"
#include "plugins/e131/e131/DMPE133Inflator.h"
#include "plugins/e131/e131/DMPHeader.h"
#include "plugins/e131/e131/DMPPDU.h"

namespace ola {
namespace plugin {
namespace e131 {

DMPE133Inflator::~DMPE133Inflator() {
  universe_handler_map::iterator rdm_iter;
  for (rdm_iter = m_rdm_handlers.begin();
       rdm_iter != m_rdm_handlers.end();
       ++rdm_iter) {
    delete rdm_iter->second;
  }
  m_rdm_handlers.clear();

  if (m_rdm_management_handler)
    delete m_rdm_management_handler;
}


/*
 * Handle a DMP PDU for E1.33.
 */
bool DMPE133Inflator::HandlePDUData(uint32_t vector,
                                    HeaderSet &headers,
                                    const uint8_t *data,
                                    unsigned int pdu_len) {
  if (vector != DMP_SET_PROPERTY_VECTOR) {
    OLA_INFO << "not a set property msg: " << vector;
    return true;
  }

  E133Header e133_header = headers.GetE133Header();
  universe_handler_map::iterator universe_iter =
      m_rdm_handlers.find(e133_header.Universe());

  if (universe_iter == m_rdm_handlers.end() && !e133_header.IsManagement())
    return true;

  DMPHeader dmp_header = headers.GetDMPHeader();

  if (!dmp_header.IsVirtual() || dmp_header.IsRelative() ||
      dmp_header.Size() != TWO_BYTES ||
      dmp_header.Type() != RANGE_EQUAL) {
    OLA_INFO << "malformed E1.33 dmp header " << dmp_header.Header();
    return true;
  }

  unsigned int available_length = pdu_len;
  std::auto_ptr<const BaseDMPAddress> address(
      DecodeAddress(dmp_header.Size(),
                    dmp_header.Type(),
                    data,
                    available_length));

  if (!address.get()) {
    OLA_INFO << "DMP address parsing failed, the length is probably too small";
    return true;
  }

  if (address->Increment() != 1) {
    OLA_INFO << "E1.33 DMP packet with increment " << address->Increment()
      << ", disarding";
    return true;
  }

  uint8_t start_code = data[available_length];
  if (start_code != ola::rdm::RDMCommand::START_CODE) {
    OLA_INFO << "Skipping packet with non RDM start code: " <<
      static_cast<unsigned int>(start_code);
    return true;
  }

  const ola::rdm::RDMRequest *request = ola::rdm::RDMRequest::InflateFromData(
    data + available_length,
    pdu_len - available_length);

  if (!request) {
    OLA_INFO << "Failed to unpack RDM Request";
    return true;
  }

  // we need to pass more info here (like the seq #, ip etc) but that can be
  // added later.
  if (e133_header.IsManagement()) {
    m_rdm_management_handler->Run(request);
  } else {
    universe_iter->second->Run(e133_header.Universe(), request);
  }
  return true;
}


/**
 * Set the RDM Handler for a universe, ownership of the handler is transferred.
 * @param universe the universe handler to remove
 * @param handler the callback to invoke when there is rdm data for this
 * universe.
 * @return true if added, false otherwise
 */
bool DMPE133Inflator::SetRDMHandler(unsigned int universe,
                                    RDMHandler *handler) {
  if (!handler)
    return false;

  RemoveRDMHandler(universe);
  m_rdm_handlers[universe] = handler;
  return true;
}


/**
 * Remove the RDM handler for a universe
 * @param universe the universe handler to remove
 * @return true if removed, false if it didn't exist
 */
bool DMPE133Inflator::RemoveRDMHandler(unsigned int universe) {
  map<unsigned int, RDMHandler*>::iterator iter =
    m_rdm_handlers.find(universe);

  if (iter != m_rdm_handlers.end()) {
    RDMHandler *old_closure = iter->second;
    m_rdm_handlers.erase(iter);
    // m_e133_layer->LeaveUniverse(universe);
    delete old_closure;
    return true;
  }
  return false;
}


/**
 * Set the mangagement RDM Handler, ownership of the handler is transferred.
 * @param handler the callback to invoke when there is mangagement rdm data for
 * this universe.
 */
void DMPE133Inflator::SetRDMManagementhandler(ManagementRDMHandler *handler) {
  RemoveRDMManagementHandler();
  m_rdm_management_handler = handler;
}


/**
 * Remove the RDM mangagement handle.
 */
void DMPE133Inflator::RemoveRDMManagementHandler() {
  if (m_rdm_management_handler)
    delete m_rdm_management_handler;
}
}  // e131
}  // plugin
}  // ola
