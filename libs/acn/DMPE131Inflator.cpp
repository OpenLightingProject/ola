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
 * DMPE131Inflator.cpp
 * The Inflator for the DMP PDUs
 * Copyright (C) 2007 Simon Newton
 */

#include <sys/time.h>
#include <algorithm>
#include <map>
#include <memory>
#include <vector>
#include "ola/Logging.h"
#include "libs/acn/DMPE131Inflator.h"
#include "libs/acn/DMPHeader.h"
#include "libs/acn/DMPPDU.h"

namespace ola {
namespace acn {

using ola::Callback0;
using ola::acn::CID;
using ola::io::OutputStream;
using std::map;
using std::pair;
using std::vector;

const TimeInterval DMPE131Inflator::EXPIRY_INTERVAL(2500000);


DMPE131Inflator::~DMPE131Inflator() {
  UniverseHandlers::iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter) {
    delete iter->second.closure;
  }
  m_handlers.clear();
}


/*
 * Handle a DMP PDU for E1.31.
 */
bool DMPE131Inflator::HandlePDUData(uint32_t vector,
                                    const HeaderSet &headers,
                                    const uint8_t *data,
                                    unsigned int pdu_len) {
  if (vector != ola::acn::DMP_SET_PROPERTY_VECTOR) {
    OLA_INFO << "not a set property msg: " << vector;
    return true;
  }

  E131Header e131_header = headers.GetE131Header();
  UniverseHandlers::iterator universe_iter =
      m_handlers.find(e131_header.Universe());

  if (e131_header.PreviewData() && m_ignore_preview) {
    OLA_DEBUG << "Ignoring preview data";
    return true;
  }

  if (universe_iter == m_handlers.end()) {
    return true;
  }

  DMPHeader dmp_header = headers.GetDMPHeader();

  if (!dmp_header.IsVirtual() || dmp_header.IsRelative() ||
      dmp_header.Size() != TWO_BYTES ||
      dmp_header.Type() != RANGE_EQUAL) {
    OLA_INFO << "malformed E1.31 dmp header " << dmp_header.Header();
    return true;
  }

  if (e131_header.Priority() > MAX_E131_PRIORITY) {
    OLA_INFO << "Priority " << static_cast<int>(e131_header.Priority())
             << " is greater than the max priority ("
             << static_cast<int>(MAX_E131_PRIORITY) << "), ignoring data";
    return true;
  }

  unsigned int available_length = pdu_len;
  std::auto_ptr<const BaseDMPAddress> address(
      DecodeAddress(dmp_header.Size(),
                    dmp_header.Type(),
                    data,
                    &available_length));

  if (!address.get()) {
    OLA_INFO << "DMP address parsing failed, the length is probably too small";
    return true;
  }

  if (address->Increment() != 1) {
    OLA_INFO << "E1.31 DMP packet with increment " << address->Increment()
             << ", disarding";
    return true;
  }

  unsigned int length_remaining = pdu_len - available_length;
  int start_code = -1;
  if (e131_header.UsingRev2()) {
    start_code = static_cast<int>(address->Start());
  } else if (length_remaining && address->Number()) {
    start_code = *(data + available_length);
  }

  // The only time we want to continue processing a non-0 start code is if it
  // contains a Terminate message.
  if (start_code && !e131_header.StreamTerminated()) {
    OLA_INFO << "Skipping packet with non-0 start code: " << start_code;
    return true;
  }

  DmxBuffer *target_buffer;
  if (!TrackSourceIfRequired(&universe_iter->second, headers,
                             &target_buffer)) {
    // no need to continue processing
    return true;
  }

  // Reaching here means that we actually have new data and we should merge.
  if (target_buffer && start_code == 0) {
    unsigned int channels = std::min(length_remaining, address->Number());
    if (e131_header.UsingRev2()) {
      target_buffer->Set(data + available_length, channels);
    } else {
      target_buffer->Set(data + available_length + 1, channels - 1);
    }
  }

  if (universe_iter->second.priority) {
    *universe_iter->second.priority = universe_iter->second.active_priority;
  }

  // merge the sources
  switch (universe_iter->second.sources.size()) {
    case 0:
      universe_iter->second.buffer->Reset();
      break;
    case 1:
      universe_iter->second.buffer->Set(
          universe_iter->second.sources[0].buffer);
      universe_iter->second.closure->Run();
      break;
    default:
      // HTP Merge
      universe_iter->second.buffer->Reset();
      std::vector<dmx_source>::const_iterator source_iter =
          universe_iter->second.sources.begin();
      for (; source_iter != universe_iter->second.sources.end();
           ++source_iter) {
        universe_iter->second.buffer->HTPMerge(source_iter->buffer);
      }
      universe_iter->second.closure->Run();
  }
  return true;
}


/*
 * Set the closure to be called when we receive data for this universe.
 * @param universe the universe to register the handler for
 * @param buffer the DmxBuffer to update with the data
 * @param handler the Callback0 to call when there is data for this universe.
 * Ownership of the closure is transferred to the node.
 */
bool DMPE131Inflator::SetHandler(uint16_t universe,
                                 ola::DmxBuffer *buffer,
                                 uint8_t *priority,
                                 ola::Callback0<void> *closure) {
  if (!closure || !buffer) {
    return false;
  }

  UniverseHandlers::iterator iter = m_handlers.find(universe);

  if (iter == m_handlers.end()) {
    universe_handler handler;
    handler.buffer = buffer;
    handler.closure = closure;
    handler.active_priority = 0;
    handler.priority = priority;
    m_handlers[universe] = handler;
  } else {
    Callback0<void> *old_closure = iter->second.closure;
    iter->second.closure = closure;
    iter->second.buffer = buffer;
    iter->second.priority = priority;
    delete old_closure;
  }
  return true;
}


/*
 * Remove the handler for this universe
 * @param universe the universe handler to remove
 * @param true if removed, false if it didn't exist
 */
bool DMPE131Inflator::RemoveHandler(uint16_t universe) {
  UniverseHandlers::iterator iter = m_handlers.find(universe);

  if (iter != m_handlers.end()) {
    Callback0<void> *old_closure = iter->second.closure;
    m_handlers.erase(iter);
    delete old_closure;
    return true;
  }
  return false;
}


/**
 * Get the list of registered universes
 * @param universes a pointer to a vector which is populated with the list of
 *   universes that have handlers installed.
 */
void DMPE131Inflator::RegisteredUniverses(vector<uint16_t> *universes) {
  universes->clear();
  UniverseHandlers::iterator iter;
  for (iter = m_handlers.begin(); iter != m_handlers.end(); ++iter) {
    universes->push_back(iter->first);
  }
}


/*
 * Check if this source is operating at the highest priority for this universe.
 * This takes care of tracking all sources for a universe at the active
 * priority.
 * @param universe_data the universe_handler struct for this universe,
 * @param HeaderSet the set of headers in this packet
 * @param buffer, if set to a non-NULL pointer, the caller should copy the data
 * in the buffer.
 * @returns true if we should remerge the data, false otherwise.
 */
bool DMPE131Inflator::TrackSourceIfRequired(
    universe_handler *universe_data,
    const HeaderSet &headers,
    DmxBuffer **buffer) {

  *buffer = NULL;  // default the buffer to NULL
  ola::TimeStamp now;
  m_clock.CurrentMonotonicTime(&now);
  const E131Header &e131_header = headers.GetE131Header();
  uint8_t priority = e131_header.Priority();
  vector<dmx_source> &sources = universe_data->sources;
  vector<dmx_source>::iterator iter = sources.begin();

  while (iter != sources.end()) {
    if (iter->cid != headers.GetRootHeader().GetCid()) {
      TimeStamp expiry_time = iter->last_heard_from + EXPIRY_INTERVAL;
      if (now > expiry_time) {
        OLA_INFO << "source " << iter->cid.ToString() << " has expired";
        iter = sources.erase(iter);
        continue;
      }
    }
    iter++;
  }

  if (sources.empty()) {
    universe_data->active_priority = 0;
  }

  for (iter = sources.begin(); iter != sources.end(); ++iter) {
    if (iter->cid == headers.GetRootHeader().GetCid()) {
      break;
    }
  }

  if (iter == sources.end()) {
    // This is an untracked source
    if (e131_header.StreamTerminated() ||
        priority < universe_data->active_priority) {
      return false;
    }

    if (priority > universe_data->active_priority) {
      OLA_INFO << "Raising priority for universe " << e131_header.Universe()
               << " from " << static_cast<int>(universe_data->active_priority)
               << " to " << static_cast<int>(priority);
      sources.clear();
      universe_data->active_priority = priority;
    }

    if (sources.size() == MAX_MERGE_SOURCES) {
      // TODO(simon): flag this in the export map
      OLA_WARN << "Max merge sources reached for universe "
               << e131_header.Universe() << ", "
               << headers.GetRootHeader().GetCid().ToString()
               << " won't be tracked";
        return false;
    } else {
      OLA_INFO << "Added new E1.31 source: "
               << headers.GetRootHeader().GetCid().ToString();
      dmx_source new_source;
      new_source.cid = headers.GetRootHeader().GetCid();
      new_source.sequence = e131_header.Sequence();
      new_source.last_heard_from = now;
      iter = sources.insert(sources.end(), new_source);
      *buffer = &iter->buffer;
      return true;
    }

  } else {
    // We already know about this one, check the seq #
    int8_t seq_diff = static_cast<int8_t>(e131_header.Sequence() -
                                          iter->sequence);
    if (seq_diff <= 0 && seq_diff > SEQUENCE_DIFF_THRESHOLD) {
      OLA_INFO << "Old packet received, ignoring, this # "
               << static_cast<int>(e131_header.Sequence()) << ", last "
               << static_cast<int>(iter->sequence);
      return false;
    }
    iter->sequence = e131_header.Sequence();

    if (e131_header.StreamTerminated()) {
      OLA_INFO << "CID " << headers.GetRootHeader().GetCid().ToString()
               << " sent a termination for universe "
               << e131_header.Universe();
      sources.erase(iter);
      if (sources.empty()) {
        universe_data->active_priority = 0;
      }
      // We need to trigger a merge here else the buffer will be stale, we keep
      // the buffer as NULL though so we don't use the data.
      return true;
    }

    iter->last_heard_from = now;
    if (priority < universe_data->active_priority) {
      if (sources.size() == 1) {
        universe_data->active_priority = priority;
      } else {
        sources.erase(iter);
        return true;
      }
    } else if (priority > universe_data->active_priority) {
      // new active priority
      universe_data->active_priority = priority;
      if (sources.size() != 1) {
        // clear all sources other than this one
        dmx_source this_source = *iter;
        sources.clear();
        iter = sources.insert(sources.end(), this_source);
      }
    }
    *buffer = &iter->buffer;
    return true;
  }
}
}  // namespace acn
}  // namespace ola
