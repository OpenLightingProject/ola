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
 * E133Receiver.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMHelper.h>

#include <string>
#include <vector>

#include "plugins/e131/e131/E133Header.h"
#include "plugins/e131/e131/DMPAddress.h"
#include "plugins/e131/e131/DMPPDU.h"
#include "plugins/e131/e131/E133Layer.h"

#include "E133Receiver.h"

using ola::plugin::e131::DMPAddressData;
using ola::plugin::e131::E133Header;
using ola::plugin::e131::TwoByteRangeDMPAddress;
using ola::rdm::RDMRequest;


E133Receiver::E133Receiver(unsigned int universe,
                           ola::rdm::RDMControllerInterface *local_controller)
    : m_e133_layer(NULL),
      m_local_controller(local_controller),
      m_universe(universe) {
  if (!universe)
    OLA_FATAL <<
      "E133Receiver created with universe 0, this isn't valid";
}


/**
 * Check for requests that have timed out
 */
void E133Receiver::CheckForStaleRequests(const ola::TimeStamp *now) {
  (void) now;
}


/**
 * Handle a RDM response addressed to this universe
 */
void E133Receiver::HandlePacket(
    const ola::plugin::e131::TransportHeader &transport_header,
    const ola::plugin::e131::E133Header &e133_header,
    const std::string &raw_request) {
  OLA_INFO << "Got data from " << transport_header.SourceIP() <<
    " for universe " << e133_header.Universe();

  // attempt to unpack as a request
  const ola::rdm::RDMRequest *request = ola::rdm::RDMRequest::InflateFromData(
    reinterpret_cast<const uint8_t*>(raw_request.data()),
    raw_request.size());

  if (!request) {
    OLA_WARN << "Failed to unpack E1.33 RDM message, ignoring request.";
    return;
  }

  m_local_controller->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &E133Receiver::RequestComplete,
                             transport_header.SourceIP(),
                             transport_header.SourcePort(),
                             e133_header.Sequence()));
}


/**
 * Handle a completed RDM request
 */
void E133Receiver::RequestComplete(ola::network::IPV4Address src_ip,
                                   uint16_t src_port,
                                   uint8_t sequence_number,
                                   ola::rdm::rdm_response_code response_code,
                                   const ola::rdm::RDMResponse *response,
                                   const std::vector<std::string> &packets) {
  if (response_code != ola::rdm::RDM_COMPLETED_OK) {
    OLA_WARN << "E1.33 request failed with code " <<
      ola::rdm::ResponseCodeToString(response_code) <<
      ", dropping request";
    delete response;
    return;
  }

  OLA_INFO << "rdm size is " << response->Size();

  // TODO(simon): handle the ack overflow case here
  // For now we just send back one packet.
  unsigned int actual_size = response->Size();
  uint8_t *rdm_data = new uint8_t[actual_size + 1];
  rdm_data[0] = ola::rdm::RDMCommand::START_CODE;

  if (!response->Pack(rdm_data + 1, &actual_size)) {
    OLA_WARN << "Failed to pack RDM response, aborting send";
    delete[] rdm_data;
    return;
  }
  unsigned int rdm_data_size = actual_size + 1;

  ola::plugin::e131::TwoByteRangeDMPAddress range_addr(0,
                                                       1,
                                                       rdm_data_size);
  ola::plugin::e131::DMPAddressData<
    ola::plugin::e131::TwoByteRangeDMPAddress> range_chunk(
        &range_addr,
        rdm_data,
        rdm_data_size);
  std::vector<DMPAddressData<TwoByteRangeDMPAddress> > ranged_chunks;
  ranged_chunks.push_back(range_chunk);
  const ola::plugin::e131::DMPPDU *pdu =
    ola::plugin::e131::NewRangeDMPSetProperty<uint16_t>(
        true,
        false,
        ranged_chunks);

  E133Header header("foo bar",
                    100,
                    sequence_number,
                    m_universe,
                    false,  // management
                    false);  // squawk

  bool result = m_e133_layer->SendDMP(header, pdu, src_ip, src_port);
  if (!result)
    OLA_WARN << "Failed to send E1.33 response";

  // send response back to src ip:port with correct seq #
  delete[] rdm_data;
  delete pdu;

  if (response)
    delete response;

  (void) src_port;
  (void) packets;
}
