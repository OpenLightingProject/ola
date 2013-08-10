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
 * ResponderSlotData.cpp
 * Manages slot data for a personality for a RDM responder.
 * Copyright (C) 2013 Peter Newman
 */

#ifndef COMMON_RDM_RESPONDERSLOTDATA_H_
#define COMMON_RDM_RESPONDERSLOTDATA_H_

#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/rdm/ResponderSlotData.h"
#include "ola/stl/STLUtils.h"

// TODO(Peter): Future work, add ability to respond with different languages for
// the slot description

namespace ola {
namespace rdm {

/**
 * Create a new chunk of slot data.
 * @param footprint the number of dmx slots consumed
 * @param description the personality name (32 chars)
 */
SlotData::SlotData(rdm_slot_type slot_type,
                   uint8_t default_slot_value,
                   const string &description)
    : m_slot_type(slot_type),
      m_default_slot_value(default_slot_value),
      m_description(description) {
}

PrimarySlotData::PrimarySlotData(rdm_slot_definition slot_definition,
                                 uint8_t default_slot_value,
                                 const string &description)
    : SlotData(ST_PRIMARY, default_slot_value, description),
      m_slot_definition(slot_definition) {
  if ((slot_definition == SD_UNDEFINED) && description.empty()) {
    OLA_WARN << "Config error, undefined slot definition and no slot "
    "description!";
  }
}

SecondarySlotData::SecondarySlotData(rdm_slot_type slot_type,
                                     uint16_t slot_definition,
                                     uint8_t default_slot_value,
                                     const string &description)
    : SlotData(slot_type, default_slot_value, description),
      m_slot_definition(slot_definition) {
  if (slot_type == ST_PRIMARY) {
    OLA_WARN << "Config error, primary slot data using the secondary slot "
    "data class!";
  }
}

SlotDataCollection::SlotDataCollection(
    const SlotDataList &slot_data)
    : m_slot_data(slot_data) {
}

uint16_t SlotDataCollection::SlotDataCount() const {
  return m_slot_data.size();
}

/**
 * Look up slot data by index
 */
SlotData *SlotDataCollection::Lookup(uint16_t slot) const {
  if (slot >= m_slot_data.size())
    return NULL;
  return m_slot_data[slot];
}
}  // namespace rdm
}  // namespace ola
#endif  // COMMON_RDM_RESPONDERSLOTDATA_H_
