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
 * E133UniverseController.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UID.h>

#include <map>
#include <string>
#include <vector>

#include "plugins/e131/e131/DMPAddress.h"
#include "plugins/e131/e131/E133Header.h"
#include "plugins/e131/e131/E133Layer.h"

#include "E133UniverseController.h"

using ola::network::IPV4Address;
using ola::plugin::e131::DMPAddressData;
using ola::plugin::e131::E133Header;
using ola::plugin::e131::TwoByteRangeDMPAddress;
using ola::rdm::RDMRequest;
using ola::rdm::UID;


E133UniverseController::E133UniverseController(unsigned int universe)
    : m_e133_layer(NULL),
      m_universe(universe) {
  if (!universe)
    OLA_FATAL <<
      "E133UniverseController created with universe 0, this isn't valid";
}


/**
 * Register the presence of a UID
 * @param target_ip the IPV4Address where the UID is located
 * @param uid the UID
 */
void E133UniverseController::AddUID(const UID &uid,
                                    const IPV4Address &target_ip) {
  uid_state_map::iterator iter = m_uid_state_map.find(uid);
  if (iter == m_uid_state_map.end()) {
    uid_state new_state = {target_ip, 0};
    m_uid_state_map[uid] = new_state;
  } else {
    m_uid_state_map[uid].ip_address = target_ip;
  }
}


/**
 * De-register a UID from this universe
 * @param uid the UID
 */
void E133UniverseController::RemoveUID(const UID &uid) {
  uid_state_map::iterator iter = m_uid_state_map.find(uid);
  if (iter == m_uid_state_map.end())
    m_uid_state_map.erase(iter);
}


/**
 * Check for requests that have timed out
 */
void E133UniverseController::CheckForStaleRequests(const ola::TimeStamp *now) {
  std::vector<std::string> raw_packets;

  while (!m_request_queue.empty()) {
    const pending_request &pending_request_s = m_request_queue.top();
    if (pending_request_s.expiry_time > *now) {
      OLA_INFO << "expired!!!";
      pending_request_s.on_complete->Run(ola::rdm::RDM_TIMEOUT,
                                         NULL,
                                         raw_packets);
      m_request_queue.pop();
    }
  }
}


/**
 * Send an RDM request
 * @param request the RDMRequest, ownership is transferred
 * @param handler the handle to call when the request completes.
 */
void E133UniverseController::SendRDMRequest(const RDMRequest *request,
                                            ola::rdm::RDMCallback *handler) {
  std::vector<std::string> raw_packets;
  if (!m_e133_layer) {
    OLA_FATAL << "e133 layer is null, UniverseController not registered!";
    handler->Run(ola::rdm::RDM_UNKNOWN_UID, NULL, raw_packets);
    delete request;
    return;
  }

  if (request->DestinationUID().IsBroadcast()) {
    SendBroadcastRequest(request, handler);
    return;
  }

  uid_state_map::iterator iter = m_uid_state_map.find(
      request->DestinationUID());
  if (iter == m_uid_state_map.end()) {
    handler->Run(ola::rdm::RDM_UNKNOWN_UID, NULL, raw_packets);
    delete request;
    return;
  }

  uint8_t *rdm_data;
  unsigned int rdm_data_size;
  if (!PackRDMRequest(request, &rdm_data, &rdm_data_size)) {
    handler->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, raw_packets);
    delete request;
    return;
  }

  // add to some sort of pending state here
  ola::TimeStamp expiry_time;
  ola::Clock::CurrentTime(&expiry_time);
  pending_request pending_request_s = {
      request,
      handler,
      expiry_time + ola::TimeInterval(1, 0)};

  if (!SendDataToUid(iter->second, rdm_data, rdm_data_size)) {
    handler->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, raw_packets);
    delete request;
  }
  m_request_queue.push(pending_request_s);
  delete[] rdm_data;
}


/**
 * Pack a RDM Request into a memory buffer including the start code.
 * @param request the RDMRequest
 * @param rdm_data pointer to a pointer of the new memory location
 * @param rdm_data_size pointer to the size of the new memory location
 * @return true if this worked, false otherwise
 */
bool E133UniverseController::PackRDMRequest(
    const ola::rdm::RDMRequest *request,
    uint8_t **rdm_data,
    unsigned int *rdm_data_size) {
  unsigned int actual_size = request->Size();
  *rdm_data = new uint8_t[actual_size + 1];
  (*rdm_data)[0] = ola::rdm::RDMCommand::START_CODE;
  if (!request->Pack(*rdm_data + 1, &actual_size)) {
    OLA_WARN << "Failed to pack RDM request, aborting send";
    delete[] *rdm_data;
    *rdm_data = NULL;
    return false;
  }
  *rdm_data_size = actual_size + 1;
  return true;
}


/**
 * Send a broadcast request
 * @param request the RDMRequest, ownership is transferred.
 * @param on_complete the handler to call when done.
 */
void E133UniverseController::SendBroadcastRequest(
    const ola::rdm::RDMRequest *request,
    ola::rdm::RDMCallback *on_complete) {
  std::vector<std::string> raw_packets;
  uid_state_map::iterator iter = m_uid_state_map.begin();
  const UID &dest_uid = request->DestinationUID();

  uint8_t *rdm_data;
  unsigned int rdm_data_size;
  if (!PackRDMRequest(request, &rdm_data, &rdm_data_size)) {
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, raw_packets);
    delete request;
    return;
  }

  if (dest_uid.ManufacturerId() == UID::ALL_MANUFACTURERS) {
    // broadcast
    for (; iter != m_uid_state_map.end(); ++iter) {
      SendDataToUid(iter->second, rdm_data, rdm_data_size);
    }
  } else {
    // vendor cast
    for (; iter != m_uid_state_map.end(); ++iter) {
      if (iter->first.ManufacturerId() == dest_uid.ManufacturerId()) {
        SendDataToUid(iter->second, rdm_data, rdm_data_size);
      }
    }
  }

  // ownership is transferred to the squawker
  QueueRequestForSquawking(request);

  on_complete->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, raw_packets);
  delete[] rdm_data;
}


/**
 * Send a block of RDM data to a UID
 * @returns true if the message was send, false otherwise
 */
bool E133UniverseController::SendDataToUid(uid_state &uid_info,
                                           const uint8_t *data,
                                           unsigned int data_size) {
  ola::plugin::e131::TwoByteRangeDMPAddress range_addr(0,
                                                       1,
                                                       (uint16_t) data_size);
  ola::plugin::e131::DMPAddressData<
    ola::plugin::e131::TwoByteRangeDMPAddress> range_chunk(
        &range_addr,
        data,
        data_size);
  std::vector<DMPAddressData<TwoByteRangeDMPAddress> > ranged_chunks;
  ranged_chunks.push_back(range_chunk);
  const ola::plugin::e131::DMPPDU *pdu =
    ola::plugin::e131::NewRangeDMPSetProperty<uint16_t>(
        true,
        false,
        ranged_chunks);

  E133Header header("foo bar",
                    100,
                    uid_info.sequence_number++,
                    m_universe,
                    false,  // management
                    false);  // squawk

  bool result = m_e133_layer->SendDMP(uid_info.ip_address,
                                      header,
                                      pdu);

  delete pdu;
  return result;
}


/**
 * Start the squawk process for this request
 * @param request the RDMRequest, ownership is transferred.
 */
void E133UniverseController::QueueRequestForSquawking(
    const ola::rdm::RDMRequest *request) {
  // TODO(simonn): finish me!
  // only squawk if this is a set request
  delete request;
}
