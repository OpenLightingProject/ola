/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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
#include "ola/strings/Format.h"
#include "ola/util/Utils.h"

namespace ola {
namespace rdm {

using ola::utils::JoinUInt8;

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
      m_muting_uid(0, 0),
      m_unmute_count(0),
      m_mute_attempts(0),
      m_tree_corrupt(false) {
}

DiscoveryAgent::~DiscoveryAgent() {
  Abort();
}

void DiscoveryAgent::Abort() {
  while (!m_uid_ranges.empty()) {
    UIDRange *range = m_uid_ranges.top();
    delete range;
    m_uid_ranges.pop();
  }

  if (m_on_complete) {
    DiscoveryCompleteCallback *callback = m_on_complete;
    m_on_complete = NULL;
    UIDSet uids;
    callback->Run(false, uids);
  }
}

void DiscoveryAgent::StartFullDiscovery(
    DiscoveryCompleteCallback *on_complete) {
  InitDiscovery(on_complete, false);
}

void DiscoveryAgent::StartIncrementalDiscovery(
    DiscoveryCompleteCallback *on_complete) {
  InitDiscovery(on_complete, true);
}

/*
 * Start the discovery process
 * @param on_complete the callback to run when discovery completes
 * @param incremental true if this is incremental, false otherwise
 */
void DiscoveryAgent::InitDiscovery(
    DiscoveryCompleteCallback *on_complete,
    bool incremental) {
  if (m_on_complete) {
    OLA_WARN << "Discovery procedure already running";
    UIDSet uids;
    on_complete->Run(false, uids);
    return;
  }
  m_on_complete = on_complete;

  // this should be empty, but clear it out anyway
  while (!m_uids_to_mute.empty()) {
    m_uids_to_mute.pop();
  }

  // this should also be empty
  while (!m_uid_ranges.empty()) {
    FreeCurrentRange();
  }

  if (incremental) {
    UIDSet::Iterator iter = m_uids.Begin();
    for (; iter != m_uids.End(); ++iter) {
      m_uids_to_mute.push(*iter);
    }
  } else {
    m_uids.Clear();
  }

  m_bad_uids.Clear();
  m_split_uids.Clear();
  m_tree_corrupt = false;

  // push the first range on to the branch stack
  UID lower(0, 0);
  m_uid_ranges.push(new UIDRange(lower, UID::AllDevices(), NULL));

  m_unmute_count = 0;
  m_target->UnMuteAll(m_unmute_callback.get());
}

/*
 * Called when the UnMute completes. This resends the Unmute command up to
 * BROADCAST_UNMUTE_REPEATS times and then starts muting previously known
 * devices (incremental only).
 */
void DiscoveryAgent::UnMuteComplete() {
  if (m_uid_ranges.empty()) {
    // Abort() was called
    return;
  }

  m_unmute_count++;
  if (m_unmute_count < BROADCAST_UNMUTE_REPEATS) {
    m_target->UnMuteAll(m_unmute_callback.get());
    return;
  }
  MaybeMuteNextDevice();
}

/*
 * If we're in incremental mode, mute previously discovered devices. Otherwise
 * proceed to the branch stage.
 */
void DiscoveryAgent::MaybeMuteNextDevice() {
  if (m_uids_to_mute.empty()) {
    SendDiscovery();
  } else {
    m_muting_uid = m_uids_to_mute.front();
    m_uids_to_mute.pop();
    OLA_DEBUG << "Muting previously discovered responder: " << m_muting_uid;
    m_target->MuteDevice(m_muting_uid, m_incremental_mute_callback.get());
  }
}

/**
 * Called when we mute a device during incremental discovery.
 */
void DiscoveryAgent::IncrementalMuteComplete(bool status) {
  if (!status) {
    m_uids.RemoveUID(m_muting_uid);
    OLA_WARN << "Unable to mute " << m_muting_uid << ", device has gone";
  } else {
    OLA_DEBUG << "Muted " << m_muting_uid;
  }
  MaybeMuteNextDevice();
}

/*
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
  if (range->uids_discovered == 0) {
    range->attempt++;
  }

  if (range->failures == MAX_BRANCH_FAILURES ||
      range->attempt == MAX_EMPTY_BRANCH_ATTEMPTS ||
      range->branch_corrupt) {
    // limit reached, move on to the next branch
    OLA_DEBUG << "Hit failure limit for (" << range->lower << ", "
              << range->upper << ")";
    if (range->parent) {
      range->parent->branch_corrupt = true;
    }
    FreeCurrentRange();
    SendDiscovery();
  } else {
    OLA_DEBUG << "DUB " << range->lower << " - " << range->upper
              << ", attempt " << range->attempt << ", uids found: "
              << range->uids_discovered << ", failures " << range->failures
              << ", corrupted " << range->branch_corrupt;
    m_target->Branch(range->lower, range->upper, m_branch_callback.get());
  }
}

/*
 * Handle a DUB response (inc. timeouts).
 * @param data the raw response, excluding the start code
 * @param length the length of the response, 0 if no response was received.
 */
void DiscoveryAgent::BranchComplete(const uint8_t *data, unsigned int length) {
  OLA_INFO << "BranchComplete, got " << length;
  if (length == 0) {
    // timeout
    if (!m_uid_ranges.empty()) {
      FreeCurrentRange();
    }
    SendDiscovery();
    return;
  }

  // Must at least have the separator, the EUID and the checksum
  if (length < 1 + EUID_SIZE + CHECKSUM_SIZE) {
    HandleCollision();
    return;
  }

  unsigned int offset = 0;
  while (data[offset] != PREAMBLE_SEPARATOR && offset < PREAMBLE_SIZE - 1) {
    if (data[offset] != PREAMBLE) {
      OLA_INFO << "Preamble " << offset << " " << strings::ToHex(data[offset]);
      HandleCollision();
      return;
    }
    offset++;
  }

  if (data[offset] != PREAMBLE_SEPARATOR) {
    OLA_INFO << "Preamble separator" << offset << " "
             << strings::ToHex(data[offset]);
    HandleCollision();
    return;
  }

  offset++;
  unsigned int remaining = length - offset;
  if (remaining < EUID_SIZE + CHECKSUM_SIZE) {
    OLA_INFO << "Insufficient data remaining, was " << remaining;
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
  for (unsigned int i = offset; i < offset + EUID_SIZE; i++) {
    calculated_checksum += data[i];
  }

  uint16_t recovered_checksum = JoinUInt8((response->ecs3 & response->ecs2),
                                          (response->ecs1 & response->ecs0));

  if (recovered_checksum != calculated_checksum) {
    OLA_INFO << "Recovered checksum: " << recovered_checksum << " != "
             << "calculated checksum: " << calculated_checksum;
    HandleCollision();
    return;
  }

  // ok this is a valid response
  uint16_t manufacturer_id = JoinUInt8((response->euid11 & response->euid10),
                                       (response->euid9 & response->euid8));
  uint32_t device_id = JoinUInt8((response->euid7 & response->euid6),
                                 (response->euid5 & response->euid4),
                                 (response->euid3 & response->euid2),
                                 (response->euid1 & response->euid0));

  UIDRange *range = m_uid_ranges.top();

  // we store this as an instance variable so we don't have to create a new
  // callback each time.
  UID located_uid = UID(manufacturer_id, device_id);
  if (m_uids.Contains(located_uid)) {
    OLA_WARN << "Previously muted responder " << located_uid
             << " continues to respond";
    range->failures++;
    if (!m_split_uids.Contains(located_uid)) {
      m_split_uids.AddUID(located_uid);
      // ignore this and continue either side of it.
      SplitAroundBadUID(located_uid);
    } else {
      // Treat this as a collision and continue branching down
      HandleCollision();
    }
  } else if (m_bad_uids.Contains(located_uid)) {
    // we've already tried this one
    OLA_INFO << "Previously bad responder " << located_uid
             << " continues to respond";
    range->failures++;
    if (!m_split_uids.Contains(located_uid)) {
      // ignore this and continue either side of it.
      SplitAroundBadUID(located_uid);
    } else {
      // Continue branching
      HandleCollision();
    }
  } else {
    m_muting_uid = located_uid;
    m_mute_attempts = 0;
    OLA_INFO << "Muting " << m_muting_uid;
    m_target->MuteDevice(m_muting_uid, m_branch_mute_callback.get());
  }
}

/*
 * Called when we successful mute a device during the branch stage.
 */
void DiscoveryAgent::BranchMuteComplete(bool status) {
  m_mute_attempts++;
  if (status) {
    m_uids.AddUID(m_muting_uid);
    m_uid_ranges.top()->uids_discovered++;
  } else {
    // failed to mute, if we haven't reached the limit try it again
    if (m_mute_attempts < MAX_MUTE_ATTEMPTS) {
      OLA_INFO << "Muting " << m_muting_uid;
      m_target->MuteDevice(m_muting_uid, m_branch_mute_callback.get());
      return;
    } else {
      // this UID is bad, either it was a phantom or it doesn't respond to
      // mute commands
      OLA_INFO << m_muting_uid << " didn't respond to MUTE, marking as bad";
      m_bad_uids.AddUID(m_muting_uid);
    }
  }
  SendDiscovery();
}

/*
 * Handle a DUB collision.
 */
void DiscoveryAgent::HandleCollision() {
  UIDRange *range = m_uid_ranges.top();
  UID lower_uid = range->lower;
  UID upper_uid = range->upper;

  if (lower_uid == upper_uid) {
    range->failures++;
    OLA_WARN << "End of tree reached!!!";
    SendDiscovery();
    return;
  }

  // work out the mid point
  uint64_t mid = (lower_uid.ToUInt64() + upper_uid.ToUInt64()) / 2;
  UID mid_uid(mid);
  mid++;
  UID mid_plus_one_uid(mid);
  OLA_INFO << "Collision, splitting into: " << lower_uid << " - " << mid_uid
           << " , " << mid_plus_one_uid << " - " << upper_uid;

  range->uids_discovered = 0;
  // add both ranges to the stack
  m_uid_ranges.push(new UIDRange(lower_uid, mid_uid, range));
  m_uid_ranges.push(new UIDRange(mid_plus_one_uid, upper_uid, range));
  SendDiscovery();
}

/*
 * Split around a bad UID.
 * This is a more specialised version of HandleCollision
 */
void DiscoveryAgent::SplitAroundBadUID(UID bad_uid) {
  UIDRange *range = m_uid_ranges.top();
  UID lower_uid = range->lower;
  UID upper_uid = range->upper;

  if (lower_uid == upper_uid) {
    range->failures++;
    OLA_WARN << "End of tree reached!!!";
    SendDiscovery();
    return;
  } else if (bad_uid < lower_uid || bad_uid > upper_uid) {
    OLA_INFO << "Bad UID " << bad_uid << " not within range " << lower_uid
             << " - " << upper_uid << ", assuming it's a phantom!";
    HandleCollision();
    return;
  }

  OLA_INFO << "Bad UID, attempting split either side of: " << bad_uid;
  UID mid_minus_one_uid(bad_uid.ToUInt64() - 1);
  UID mid_plus_one_uid(bad_uid.ToUInt64() + 1);

  range->uids_discovered = 0;
  if (mid_minus_one_uid >= lower_uid) {
    OLA_INFO << "Splitting either side of " << bad_uid << ", adding "
             << lower_uid << " - " << mid_minus_one_uid;
    m_uid_ranges.push(new UIDRange(lower_uid, mid_minus_one_uid, range));
  }
  if (mid_plus_one_uid <= upper_uid) {
    OLA_INFO << "Splitting either side of " << bad_uid << ", adding "
             << mid_plus_one_uid << " - " << upper_uid;
    m_uid_ranges.push(new UIDRange(mid_plus_one_uid, upper_uid, range));
  }
  SendDiscovery();
}

/*
 * Deletes the current range from the stack, and pops it.
 */
void DiscoveryAgent::FreeCurrentRange() {
  UIDRange *range = m_uid_ranges.top();
  if (m_uid_ranges.size() == 1) {
    // top of stack
    if (range->branch_corrupt) {
      OLA_INFO << "Discovery tree is corrupted";
      m_tree_corrupt = true;
    }
  } else {
    range->parent->uids_discovered += range->uids_discovered;
  }
  delete range;
  m_uid_ranges.pop();
}
}  // namespace rdm
}  // namespace ola
