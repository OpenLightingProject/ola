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
 * RDMInflator.cpp
 * The Inflator for the DMP PDUs
 * Copyright (C) 2011 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <map>
#include <memory>
#include <string>
#include "ola/Logging.h"
#include "ola/rdm/RDMCommand.h"
#include "plugins/e131/e131/RDMInflator.h"
#include "plugins/e131/e131/DMPHeader.h"
#include "plugins/e131/e131/DMPPDU.h"

namespace ola {
namespace plugin {
namespace e131 {

using std::string;

/**
 * Create a new RDM inflator
 * @param on_data a callback which is run when ever we receive a packet. This
 * is used for healthchecking but should be removed.
 */
RDMInflator::RDMInflator(OnDataCallback *on_data)
    : BaseInflator(PDU::FOUR_BYTES),
      m_on_data(on_data) {
}


/**
 * Clean up this inflator
 */
RDMInflator::~RDMInflator() {
  endpoint_handler_map::iterator rdm_iter;
  for (rdm_iter = m_rdm_handlers.begin();
       rdm_iter != m_rdm_handlers.end();
       ++rdm_iter) {
    delete rdm_iter->second;
  }
  m_rdm_handlers.clear();
}


/**
 * Set the RDM Handler for an endpoint, ownership of the handler is
 * transferred.
 * @param endpoint the endpoint to use the handler for
 * @param handler the callback to invoke when there is rdm data for this
 * universe.
 * @return true if added, false otherwise
 */
bool RDMInflator::SetRDMHandler(uint16_t endpoint,
                                RDMMessageHandler *handler) {
  if (!handler)
    return false;

  RemoveRDMHandler(endpoint);
  m_rdm_handlers[endpoint] = handler;
  return true;
}


/**
 * Remove the RDM handler for an endpoint
 * @param endpoint the endpoint to remove the handler for.
 * @return true if removed, false if it didn't exist
 */
bool RDMInflator::RemoveRDMHandler(uint16_t endpoint) {
  endpoint_handler_map::iterator iter = m_rdm_handlers.find(endpoint);

  if (iter != m_rdm_handlers.end()) {
    delete iter->second;
    m_rdm_handlers.erase(iter);
    return true;
  }
  return false;
}


/*
 * Decode the RDM 'header', which is 0 bytes in length.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool RDMInflator::DecodeHeader(HeaderSet&,
                               const uint8_t*,
                               unsigned int,
                               unsigned int &bytes_used) {
  bytes_used = 0;
  return true;
}


/*
 * Handle a DMP PDU for E1.33.
 */
bool RDMInflator::HandlePDUData(uint32_t vector,
                                HeaderSet &headers,
                                const uint8_t *data,
                                unsigned int pdu_len) {
  // if we have a callback registered notify that we got some data
  if (m_on_data)
    m_on_data->Run(headers.GetTransportHeader());

  if (vector != RDM_DATA_VECTOR) {
    OLA_INFO << "Not a RDM message, vector was " << vector;
    return true;
  }

  E133Header e133_header = headers.GetE133Header();
  endpoint_handler_map::iterator endpoint_iter =
      m_rdm_handlers.find(e133_header.Endpoint());

  if (endpoint_iter == m_rdm_handlers.end()) {
    if (!e133_header.Endpoint()) {
      OLA_WARN << "Received E1.33 message for Endpoint 0 but no handler set!";
    } else {
      OLA_DEBUG << "Received E1.33 message for Endpoint " <<
        e133_header.Endpoint() << ", no handler set";
    }
    return true;
  }

  uint8_t start_code = data[0];
  if (start_code != ola::rdm::RDMCommand::START_CODE) {
    OLA_INFO << "Skipping packet with non RDM start code: " <<
      static_cast<unsigned int>(start_code);
    return true;
  }

  string rdm_message(reinterpret_cast<const char*>(&data[1]),
                     pdu_len - 1);

  endpoint_iter->second->Run(headers.GetTransportHeader(),
                             e133_header,
                             rdm_message);
  return true;
}
}  // e131
}  // plugin
}  // ola
