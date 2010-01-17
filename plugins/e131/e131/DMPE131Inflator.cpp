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
#include <vector>
#include "ola/Logging.h"
#include "plugins/e131/e131/DMPE131Inflator.h"
#include "plugins/e131/e131/DMPHeader.h"
#include "plugins/e131/e131/DMPPDU.h"

namespace ola {
namespace plugin {
namespace e131 {

using std::map;
using std::pair;
using std::vector;
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
  map<unsigned int, universe_handler>::iterator universe_iter =
      m_handlers.find(e131_header.Universe());

  if (e131_header.PreviewData() && m_ignore_preview) {
    OLA_INFO << "Ignoring preview data";
    return true;
  }

  if (universe_iter == m_handlers.end())
    return true;

  DMPHeader dmp_header = headers.GetDMPHeader();

  if (!dmp_header.IsVirtual() || dmp_header.IsRelative() ||
      dmp_header.Size() != TWO_BYTES ||
      dmp_header.Type() != RANGE_EQUAL) {
    OLA_INFO << "malformed E1.31 dmp header " << dmp_header.Header();
    return true;
  }

  if (e131_header.Priority() >= MAX_PRIORITY) {
    OLA_INFO << "Priority " << e131_header.Priority() <<
      " is greater than the max priority (" << MAX_PRIORITY <<
      "), ignoring data";
    return true;
  }

  unsigned int available_length = pdu_len;
  const BaseDMPAddress *address = DecodeAddress(dmp_header.Size(),
                                                dmp_header.Type(),
                                                data,
                                                available_length);
  if (address->Increment() != 1) {
    OLA_INFO << "E1.31 DMP packet with increment " << address->Increment()
      << ", disarding";
    return true;
  }

  if (!TrackSourceIfRequired(universe_iter->second, headers))
    // no need to continue processing
    return true;

  // This implies that we actually have new data and we should merge.

  unsigned int channels = std::min(pdu_len - available_length,
                                   address->Number());
  if (e131_header.UsingRev2()) {
    // drop non-0 start codes
    if (address->Start() == 0) {
      universe_iter->second.buffer->Set(data + available_length, channels);
      universe_iter->second.closure->Run();
    }
  } else {
    // skip non-0 start codes
    if (*(data + available_length) == 0 && channels > 0) {
      universe_iter->second.buffer->Set(data + available_length + 1,
                                        channels - 1);
      universe_iter->second.closure->Run();
    }
  }
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
    handler.active_priority = 0;
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


/*
 * Check if this source is operating at the highest priority for this universe.
 * This takes care of tracking all sources for a universe at the active
 * priority.
 * @returns true is this universe is operating at the highest priority, false
 * otherwise.
 */
bool DMPE131Inflator::TrackSourceIfRequired(
    universe_handler &universe_data,
    const HeaderSet &headers) {

  const E131Header &e131_header = headers.GetE131Header();
  vector<dmx_source> &sources = universe_data.sources;
  vector<dmx_source>::iterator iter;
  vector<dmx_source>::iterator source = sources.end();

  for (iter = sources.begin(); iter != sources.end(); iter++) {
    // clean out stale entries here

    if (iter->cid == headers.GetRootHeader().GetCid())
      source = iter;
  }

  if (!sources.size())
    universe_data.active_priority = 0;

  uint8_t priority = e131_header.Priority();

  if (source == sources.end()) {
    // This is an untracked source
    if (e131_header.StreamTerminated() ||
        priority < universe_data.active_priority)
      return false;

    if (priority > universe_data.active_priority) {
      OLA_INFO << "Raising priority for " <<
        e131_header.Universe() << " from " <<
        universe_data.active_priority << " to " << priority;
      sources.clear();
      universe_data.active_priority = priority;
    }

    if (sources.size() == MAX_MERGE_SOURCES) {
      OLA_INFO << "Max merge sources reached for universe " <<
        e131_header.Universe() << ", " <<
        headers.GetRootHeader().GetCid().ToString() << " won't be tracked";
        return false;
    } else {
      OLA_INFO << "Added new E1.31 source: " <<
        headers.GetRootHeader().GetCid().ToString();
      dmx_source new_source;
      new_source.cid = headers.GetRootHeader().GetCid();
      new_source.sequence = e131_header.Sequence();
      sources.push_back(new_source);
      return true;
    }
  } else {
    // We already know about this one, check the seq #
    int8_t seq_diff = e131_header.Sequence() - source->sequence;
    if (seq_diff <= 0 && seq_diff > SEQUENCE_DIFF_THRESHOLD) {
      OLA_INFO << "Old packet received, ignoring, this # " <<
        static_cast<int>(e131_header.Sequence()) << ", last " <<
        static_cast<int>(source->sequence);
      return false;
    }
    source->sequence = e131_header.Sequence();

    // This is an untracked source
    if (e131_header.StreamTerminated()) {
      sources.erase(source);
      if (!sources.size())
        universe_data.active_priority = 0;
      // We need to trigger a merge here else the buffer will be stale
      return true;
    }

    if (priority < universe_data.active_priority) {
      if (sources.size() == 1)
        universe_data.active_priority = priority;
      else
        sources.erase(source);
    } else if (priority > universe_data.active_priority) {
      // new active priority
      universe_data.active_priority = priority;
      if (sources.size() != 1) {
        // clear all sources other than this one
        dmx_source this_source = *source;
        sources.clear();
        sources.push_back(this_source);
      }
    }
    return true;
  }
}
}  // e131
}  // plugin
}  // ola
