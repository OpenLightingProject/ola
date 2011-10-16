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
 * DiscoveryAgent.cpp
 * Implements the RDM Discovery algorithm.
 * Copyright (C) 2011 Simon Newton
 */

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/rdm/DiscoveryAgent.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "string"

namespace ola {
namespace rdm {


/**
 * Create a new DiscoveryAgent
 * @param target the DiscoveryTargetInterface to use for sending messages.
 */
DiscoveryAgent::DiscoveryAgent(DiscoveryTargetInterface *target)
    : m_target(target),
      m_on_complete(NULL),
      m_unmute_callback(
          ola::NewCallback(this, &DiscoveryAgent::UnMuteComplete)),
      m_incremental_mute_callback(
        ola::NewCallback(this, &DiscoveryAgent::IncrementalMuteComplete)),
      m_branch_mute_callback(
        ola::NewCallback(this, &DiscoveryAgent::BranchMuteComplete)),
      m_branch_callback(
        ola::NewCallback(this, &DiscoveryAgent::BranchComplete)),
      m_muting_uid(0, 0) {
}


/**
 * Clean up
 */
DiscoveryAgent::~DiscoveryAgent() {
  delete m_unmute_callback;
  delete m_incremental_mute_callback;
  delete m_branch_mute_callback;
  delete m_branch_callback;
}


/**
 * Initiate full discovery
 * @param on_complete the callback to run once discovery completes.
 */
void DiscoveryAgent::StartFullDiscovery(
    DiscoveryCompleteCallback *on_complete) {
  InitDiscovery(on_complete, false);
}


/**
 * Initiate incremental discovery
 * @param on_complete the callback to run once discovery completes.
 */
void DiscoveryAgent::StartIncrementalDiscovery(
    DiscoveryCompleteCallback *on_complete) {
  InitDiscovery(on_complete, true);
}


/**
 * Start the discovery process
 * @param on_complete the callback to run when discovery completes
 * @param incremental true if this is incremental, false otherwise
 */
void DiscoveryAgent::InitDiscovery(
    DiscoveryCompleteCallback *on_complete,
    bool incremental) {
  if (m_on_complete) {
    UIDSet uids;
    OLA_WARN << "Discovery already running";
    on_complete->Run(false, uids);
    return;
  }
  m_on_complete = on_complete;

  while (!m_uids_to_mute.empty())
    m_uids_to_mute.pop();

  while (!m_uid_ranges.empty())
    FreeCurrentRange(false);

  if (incremental) {
    UIDSet::Iterator iter = m_uids.Begin();
    for (;iter != m_uids.End(); ++iter)
      m_uids_to_mute.push(*iter);
  } else {
    m_uids.Clear();
  }

  m_tree_corrupt = false;

  // push the first ranges on to the branch stack
  UID lower(0, 0);
  m_uid_ranges.push(new UIDRange(lower, UID::AllDevices(), NULL));
  m_uid_ranges.push(new UIDRange(lower, UID::AllDevices(), NULL));

  m_target->UnMuteAll(m_unmute_callback);
}


/**
 * Called when we mute a device during incremental discovery.
 */
void DiscoveryAgent::IncrementalMuteComplete(bool status) {
  OLA_INFO << "mute complete";
  if (!status) {
    m_uids.RemoveUID(m_muting_uid);
    OLA_WARN << "Mute of " << m_muting_uid << " failed, device has gone";
  }
  MaybeMuteNextDevice();
}


/**
 * Called when the UnMute completes
 */
void DiscoveryAgent::UnMuteComplete() {
  MaybeMuteNextDevice();
}


/**
 * Called when we get a response (or timeout) to a branch request.
 * @param data the raw response, excluding the start code
 * @param length the length of the response, 0 if no response was received.
 */
void DiscoveryAgent::BranchComplete(const uint8_t *data, unsigned int length) {
  if (length == 0) {
    // timeout
    FreeCurrentRange(false);
    SendDiscovery();
    return;
  }

  if (length < MIN_DUB_RESPONSE_SIZE || length > MAX_DUB_RESPONSE_SIZE) {
    HandleCollision();
    return;
  }

  unsigned int preamble_size = length - MIN_DUB_RESPONSE_SIZE;
  for (unsigned int i = 0; i < preamble_size; i++) {
    if (data[i] != 0xfe) {
      OLA_INFO << "preamble " << i << " " << std::hex <<
        static_cast<int>(data[i]);
      HandleCollision();
      return;
    }
  }

  unsigned int offset = preamble_size;
  if (data[offset++] != 0xaa) {
    OLA_INFO << "preamble separator is " << std::hex <<
      static_cast<int>(data[offset]);
    HandleCollision();
    return;
  }

  typedef struct {
    uint8_t euid11;
    uint8_t euid10;
    uint8_t euid9;
    uint8_t euid8;
    uint8_t euid7;
    uint8_t euid6;
    uint8_t euid5;
    uint8_t euid4;
    uint8_t euid3;
    uint8_t euid2;
    uint8_t euid1;
    uint8_t euid0;
    uint8_t ecs3;
    uint8_t ecs2;
    uint8_t ecs1;
    uint8_t ecs0;
  } dub_response_structure;

  const dub_response_structure *response =
      reinterpret_cast<const dub_response_structure*>(data + offset);

  uint16_t calculated_checksum = 0;
  for (unsigned int i = offset; i < offset + 12; i++)
    calculated_checksum += data[i];

  uint16_t recovered_checksum =
    ((response->ecs3 & response->ecs2) << 8) +
    (response->ecs1 & response->ecs0);

  if (recovered_checksum != calculated_checksum) {
    OLA_INFO << "recovered checksum: " << recovered_checksum << " != " <<
      "calculated checksum: " << calculated_checksum;
    HandleCollision();
    return;
  }

  // ok this is a valid response
  uint16_t manufacturer_id =
    ((response->euid11 & response->euid10) << 8) +
    (response->euid9 & response->euid8);
  uint32_t device_id =
    ((response->euid7 & response->euid6) << 24) +
    ((response->euid5 & response->euid4) << 16) +
    ((response->euid3 & response->euid2) << 8) +
    (response->euid1 & response->euid0);

  // we store this as an instance variable so we don't have to create a new
  // callback each time.
  UID located_uid = UID(manufacturer_id, device_id);
  if (m_uids.Contains(located_uid)) {
    OLA_WARN << "Previous muted responder " << located_uid <<
      " continues to respond";
    // ignore this and continue on to the next branch.
    SendDiscovery();
  } else {
    m_muting_uid = located_uid;
    OLA_INFO << "muting " << m_muting_uid;
    m_target->MuteDevice(m_muting_uid, m_branch_mute_callback);
  }
}


/**
 * Called when we successfull mute a device during the branch phase
 */
void DiscoveryAgent::BranchMuteComplete(bool status) {
  if (status)
    m_uids.AddUID(m_muting_uid);

  FreeCurrentRange(true);
  SendDiscovery();
}


/**
 * If any previously discovered devices remain, mute them. Otherwise proceed to
 * the branch phase.
 */
void DiscoveryAgent::MaybeMuteNextDevice() {
  if (m_uids_to_mute.empty()) {
    SendDiscovery();
  } else {
    m_muting_uid = m_uids_to_mute.front();
    m_uids_to_mute.pop();
    OLA_INFO << "muting " << m_muting_uid;
    m_target->MuteDevice(m_muting_uid, m_incremental_mute_callback);
  }
}


/**
 * Send a Discovery Unique Branch request.
 */
void DiscoveryAgent::SendDiscovery() {
  if (m_uid_ranges.empty()) {
    // we're hit the end of the stack, now we're done
    if (m_on_complete) {
      m_on_complete->Run(!m_tree_corrupt, m_uids);
      m_on_complete = NULL;
    } else {
      OLA_WARN << "Discovery complete but no callback";
    }
    return;
  }
  UIDRange *range = m_uid_ranges.top();
  range->attempt++;
  OLA_INFO << "DUB " << range->lower << " - " << range->upper << ", attempt "
    << range->attempt << ", corrupted " << range->branch_corrupt;

  if (range->attempt == MAX_BRANCH_ATTEMPTS || range->branch_corrupt) {
    // limit reached, move on to the next branch
    if (range->parent)
      range->parent->branch_corrupt = true;
    FreeCurrentRange(false);
    SendDiscovery();
  } else {
    m_target->Branch(range->lower, range->upper, m_branch_callback);
  }
}


/**
 * Handle a collision
 */
void DiscoveryAgent::HandleCollision() {
  UIDRange *range = m_uid_ranges.top();
  UID lower_uid = range->lower;
  UID upper_uid = range->upper;

  if (lower_uid == upper_uid) {
    OLA_WARN << "end of tree reached!!!";
    return;
  }

  // work out the mid point
  uint64_t lower = ((static_cast<uint64_t>(lower_uid.ManufacturerId()) << 32) +
                    lower_uid.DeviceId());
  uint64_t upper = ((static_cast<uint64_t>(upper_uid.ManufacturerId()) << 32) +
                    upper_uid.DeviceId());
  uint64_t mid = (lower + upper) / 2;
  UID mid_uid(mid >> 32, mid);
  mid++;
  UID mid_plus_one_uid(mid >> 32, mid);
  OLA_INFO << " collision, spliting into: " << lower_uid << " - " << mid_uid <<
    " , " << mid_plus_one_uid << " - " << upper_uid;

  // add both ranges to the stack
  m_uid_ranges.push(new UIDRange(lower_uid, mid_uid, range));
  m_uid_ranges.push(new UIDRange(mid_plus_one_uid, upper_uid, range));
  SendDiscovery();
}


/**
 * Deletes the current range from the stack, and pops it.
 */
void DiscoveryAgent::FreeCurrentRange(bool got_response_in_current_range) {
  UIDRange *range = m_uid_ranges.top();
  if (m_uid_ranges.size() == 1) {
    // top of stack
    if (got_response_in_current_range) {
      OLA_INFO << "not freeeing, going round again";
      return;
    } else if (range->branch_corrupt) {
      OLA_INFO << "top of tree is corrupted";
      m_tree_corrupt = true;
    }
  }
  delete range;
  m_uid_ranges.pop();
}
}  // rdm
}  // ola
