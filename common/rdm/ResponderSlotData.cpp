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

namespace ola {
namespace rdm {

/**
 * Create a new chunk of slot data.
 * @param footprint the number of dmx slots consumed
 * @param description the personality name (32 chars)
 */
SlotData::SlotData(rdm_slot_type slot_type,
                   rdm_slot_definition slot_definition,
                   uint8_t default_slot_value,
                   const string &description)
    : m_slot_type(slot_type),
      m_slot_definition(slot_definition),
      m_default_slot_value(default_slot_value),
      m_description(description) {
OLA_DEBUG << "Intialised slot data as type " << m_slot_type << ", def " << m_slot_definition << ", default val " << static_cast<int>(m_default_slot_value) << ", desc " << m_description;
}

/**
 * Takes ownership of the slot data
 */
SlotDataCollection::SlotDataCollection(
    const SlotDataList &slot_data)
    : m_slot_data(slot_data) {
}

/**
 * Clean up
 */
SlotDataCollection::~SlotDataCollection() {
}

uint16_t SlotDataCollection::SlotDataCount() const {
  return m_slot_data.size();
}

/**
 * Look up slot data by index
 */
const SlotData *SlotDataCollection::Lookup(uint16_t slot) const {
  OLA_DEBUG << "Looking for slot data for slot " << slot << " of a total of " << m_slot_data.size();
  if (slot >= m_slot_data.size())
    return NULL;
  OLA_DEBUG << "Looking at slot data as type " << static_cast<int>(m_slot_data[slot].SlotType()) << ", def " << static_cast<uint16_t>(m_slot_data[slot].SlotDefinition()) << ", default val " << static_cast<uint16_t>(m_slot_data[slot].DefaultSlotValue());
// << ", desc " << m_slot_data[slot]->Description();
  return &m_slot_data[slot];
}

//SlotDataManager::SlotDataManager(
//    const SlotDataCollection *slot_data)
//    : m_slot_data(slot_data) {
//}

//uint16_t SlotDataManager::SlotDataCount() const {
//  return m_slot_data->SlotDataCount();
//}

// Lookup some slot data.
//const SlotData *SlotDataManager::Lookup(uint16_t slot) const {
//  return m_slot_data->Lookup(slot);
//}

}  // namespace rdm
}  // namespace ola
#endif  // COMMON_RDM_RESPONDERSLOTDATA_H_
