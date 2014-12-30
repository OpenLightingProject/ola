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
 * ResponderSlotData.cpp
 * Manages slot data for a personality for a RDM responder.
 * Copyright (C) 2013 Peter Newman
 */

#ifndef COMMON_RDM_RESPONDERSLOTDATA_H_
#define COMMON_RDM_RESPONDERSLOTDATA_H_

#include <string>

#include "ola/Logging.h"
#include "ola/rdm/ResponderSlotData.h"

namespace ola {
namespace rdm {

using std::string;

SlotData SlotData::PrimarySlot(rdm_slot_definition slot_definition,
                               uint8_t default_slot_value) {
  if (slot_definition == SD_UNDEFINED) {
    OLA_WARN << "Undefined slot definition and no slot description!";
  }
  return SlotData(ST_PRIMARY, slot_definition, default_slot_value);
}

SlotData SlotData::PrimarySlot(rdm_slot_definition slot_definition,
                               uint8_t default_slot_value,
                               const string &description) {
  if (slot_definition == SD_UNDEFINED && description.empty()) {
    OLA_WARN << "Undefined slot definition and no slot description!";
  }
  return SlotData(ST_PRIMARY, slot_definition, default_slot_value, description);
}

SlotData SlotData::SecondarySlot(rdm_slot_type slot_type,
                                 uint16_t primary_slot,
                                 uint8_t default_slot_value) {
  if (slot_type == ST_PRIMARY) {
    OLA_WARN << "Secondary slot created with slot_type == ST_PRIMARY";
  }
  return SlotData(slot_type, primary_slot, default_slot_value);
}

SlotData SlotData::SecondarySlot(rdm_slot_type slot_type,
                                 uint16_t primary_slot,
                                 uint8_t default_slot_value,
                                 const string &description) {
  if (slot_type == ST_PRIMARY) {
    OLA_WARN << "Secondary slot created with slot_type == ST_PRIMARY: "
             << description;
  }
  return SlotData(slot_type, primary_slot, default_slot_value, description);
}

SlotData::SlotData(rdm_slot_type slot_type,
                   uint16_t slot_id,
                   uint8_t default_slot_value)
    : m_slot_type(slot_type),
      m_slot_id(slot_id),
      m_default_slot_value(default_slot_value),
      m_has_description(false) {
}

SlotData::SlotData(rdm_slot_type slot_type,
                   uint16_t slot_id,
                   uint8_t default_slot_value,
                   const string &description)
    : m_slot_type(slot_type),
      m_slot_id(slot_id),
      m_default_slot_value(default_slot_value),
      m_has_description(true),
      m_description(description) {
}

SlotDataCollection::SlotDataCollection(const SlotDataList &slot_data)
    : m_slot_data(slot_data) {
}

uint16_t SlotDataCollection::SlotCount() const {
  return m_slot_data.size();
}

const SlotData *SlotDataCollection::Lookup(uint16_t slot) const {
  if (slot >= m_slot_data.size())
    return NULL;
  return &m_slot_data[slot];
}
}  // namespace rdm
}  // namespace ola
#endif  // COMMON_RDM_RESPONDERSLOTDATA_H_
